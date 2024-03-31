/****************************************************************************
Copyright (c) 2016 Wi-Fi Alliance.  All Rights Reserved

Permission to use, copy, modify, and/or distribute this software for any purpose with or
without fee is hereby granted, provided that the above copyright notice and this permission
notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************************/

/*
 *   File: wfa_cs.c -- configuration and setup
 *   This file contains all implementation for the dut setup and control
 *   functions, such as network interfaces, ip address and wireless specific
 *   setup with its supplicant.
 *
 *   The current implementation is to show how these functions
 *   should be defined in order to support the Agent Control/Test Manager
 *   control commands. To simplify the current work and avoid any GPL licenses,
 *   the functions mostly invoke shell commands by calling linux system call,
 *   system("<commands>").
 *
 *   It depends on the differnt device and platform, vendors can choice their
 *   own ways to interact its systems, supplicants and process these commands
 *   such as using the native APIs.
 *
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef _FREERTOS
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <linux/if_bridge.h>
#include <poll.h>
#else
#include <fcntl.h>
#include "lwip/sockets.h"
#include "lwip/inet.h"
#endif

#include "wfa_portall.h"
#include "wfa_debug.h"
#include "wfa_ver.h"
#include "wfa_main.h"
#include "wfa_types.h"
#include "wfa_ca.h"
#include "wfa_tlv.h"
#include "wfa_sock.h"
#include "wfa_tg.h"
#include "wfa_cmds.h"
#include "wfa_rsp.h"
#include "wfa_utils.h"
#ifdef _FREERTOS
#include "wfa_miscs.h"
#include "cli.h"
#include "ping.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "gl_wifi_cli.h"
#include "wifi_api.h"
#endif
#ifdef WFA_WMM_PS_EXT
#include "wfa_wmmps.h"
#endif
#include "wpa_helpers.h"
#include "wpa_ctrl.h"


/**
 * Forward declaration for internal utilities
 */
#ifdef _FREERTOS
#define system shell
#endif

#define AP_BRIDGE_INF_NAME "br0"
#define USB0_INF_NAME "rndis0"
#define USB1_INF_NAME "usb0"

#define UNSET_VALUE -1

int Totalpkts = 0;
char g_rssi[8] = {0};

typedef struct sta_params {
    caStaSetIpConfig_t ipconfig;
    char nonPrefChanStr[WFA_BUFF_64];
    /* use array below this line. Must sync with set_params and get_params */
    char intf[WFA_IF_NAME_LEN];
    int type[1];
    char ssid[WFA_SSID_NAME_LEN];
    char keyMgmtType[16];
    char encpType[16];
    int pmf[1];
    int akmSuiteType[1];
    char clientCertificate[128];
    char certType[16];
    char ecGroupID[64];
    char groupCipher[16];
    char groupMgntCipher[16];
    char innerEAP[16];
    char invalidSAEElement[64];
    char networkMode[16];
    int owe[1];
    char pacFile[32];
    char pairwiseCipher[16];
    char passphrase[64];
    char password[96];
    int pmksaCaching[1];
    int profile[1];
    char prog[16];
    char program[16];
    char trustedRootCA[128];
    char tlsCipher[64];
    char username[64];
    int peapVersion[1];
    int prefer[1];
    char micAlg[16];
    char key1[32];
    char key2[32];
    char key3[32];
    char key4[32];
    int activeKeyIdx[1];
    int bcc_mode[1];
    int mcs_fixedrate[1];
    int mcs_fixedrate_rate[1];
    int bw_sgnl[1];
    int saePwe[1];
} sta_params_t;

#ifdef CONFIG_MTK_AP
typedef struct ap_params {
    char hostapd_bin[64];
    char hostapd_conf[64];
    char driver_conf[64];
    char inf_name[WFA_IF_NAME_LEN];
    char ctrl_inf[64];

    char ip_addr[WFA_IP_ADDR_STR_LEN];
    char ip_netmask[WFA_IP_MASK_STR_LEN];

    char device_name[WFA_IF_NAME_LEN];
    char ssid[WFA_SSID_NAME_LEN];
    enum ENUM_AP_MODE mode;
    int channel;
    int rts;
    int frgmnt;
    int bcnint;
    int dtim_period;
    enum ENUM_CHANNEL_WIDTH ch_width;
    enum ENUM_CHANNEL_OFFSET ch_offset;
    enum wfa_state pmf;
    char passphrase[101];
    char wepkey[27];
    enum wfa_state wme;
    enum wfa_state wmmps;
    enum wfa_state p2p_mgmt;
    char country_code[3];
    enum wfa_state sgi20;
    enum wfa_state sig_rts;
    enum wfa_state dynamic_bw_signaling;
    enum wfa_state preauthentication;
    enum wfa_state sha256ad;
    enum ENUM_KEY_MGNT_TYPE keyMgmtType;
    enum ENUM_ENCP_TYPE encpType;
    enum ENUM_PROGRAME_TYPE program;
    char ap_ecGroupID[64];
    int antiCloggingThreshold;
    int ap_reflection;
    char ap_sae_commit_override[1024];
    int bcc_mode;
    int mcs_fixedrate;
    int mcs_fixedrate_rate;
} ap_params_t;
#endif

#ifdef CONFIG_MTK_P2P
typedef struct p2p_params {
    char current_ssid[WFA_SSID_NAME_LEN];
    char p2p_ifname[IFNAMSIZ];
    enum wfa_state persistent;
    enum ENUM_P2P_MODE p2p_mode;
    enum WPS_METHOD wps_method;
    char wpsPin[WFA_WPS_PIN_LEN];
    int stop_event_thread;
#ifdef CONFIG_MTK_WFD
    int group_started;
    int role; //0: CLIENT, 1: GO, -1: undefined
#endif
} p2p_params_t;
#endif

typedef struct mtk_dut
{
    char shellResult[WFA_CMD_STR_SZ * 4];

    int networkId;
    int sta_pmf;
    int program;
    char ssid[WFA_SSID_NAME_LEN];

    enum ENUM_DRIVER_GEN driver_gen;
    sta_params_t sta_params; // params from ucc
#ifdef CONFIG_MTK_AP
    ap_params_t ap_params;
#endif
#ifdef CONFIG_MTK_P2P
    p2p_params_t p2p_params;
#endif

    int inBandTxOPEnhanceIteration;
#ifdef _FREERTOS
    int trafficIteration;
#endif
} mtk_dut_t;

mtk_dut_t gDut;
char *sigma_mode_tbl[] = {
    MODE_WMM_PS,        /* 0 */
    MODE_WMM_AC,        /* 1 */
    MODE_VOE,           /* 2 */
    MODE_P2P,           /* 3 */
    MODE_AP,            /* 4 */
    MODE_TDLS,          /* 5 */
    MODE_TG_N,          /* 6 */
    MODE_TG_AC,         /* 7 */
    MODE_WPA3,          /* 8 */
    MODE_MBO,           /* 9 */
    MODE_PMF,           /* 10 */
    MODE_TG_AX,         /* 11 */
    MODE_WFD,            /* 12 */
};
enum {
    SIGMA_MODE_WMM_PS,  /* 0 */
    SIGMA_MODE_WMM_AC,  /* 1 */
    SIGMA_MODE_VOE,     /* 2 */
    SIGMA_MODE_P2P,     /* 3 */
    SIGMA_MODE_AP,      /* 4 */
    SIGMA_MODE_TDLS,    /* 5 */
    SIGMA_MODE_TG_N,    /* 6 */
    SIGMA_MODE_TG_AC,   /* 7 */
    SIGMA_MODE_WPA3,    /* 8 */
    SIGMA_MODE_MBO,     /* 9 */
    SIGMA_MODE_PMF,     /* 10 */
    SIGMA_MODE_TG_AX,   /* 11 */
    SIGMA_MODE_WFD,     /* 12 */
};

#if defined(_FREERTOS)
int g_sigma_mode_max = sizeof(sigma_mode_tbl) / sizeof(*sigma_mode_tbl);
#endif

const char *prog_str_tbl[] = {
    "NONE",             /* 0 */
    "General",          /* PROG_TYPE_GEN */
    "PMF",              /* PROG_TYPE_PMF */
    "TDLS",             /* PROG_TYPE_TDLS */
    "VOE",              /* PROG_TYPE_VENT */
    "WFD",              /* PROG_TYPE_WFD */
    "WFDS",             /* PROG_TYPE_WFDS */
    "HS2",              /* PROG_TYPE_HS2 */
    "HS2-R2",           /* PROG_TYPE_HS2_R2 */
    "NAN",              /* PROG_TYPE_NAN */
    "LOC",              /* PROG_TYPE_LOC */
    "MBO",              /* PROG_TYPE_MBO */
    "HE",               /* PROG_TYPE_HE */
};

int sigma_mode = SIGMA_MODE_WMM_AC;

int ascii2hexstr(const char *str, char *hex);
int channel2freq(char channel);
const char* prog2str(int prog);
int str2prog(const char *str);
char* encpType2str(int encpType);
int str2encpType(const char *str);
void wfa_cs_init(void);
int chk_exist(const char *path);
int shell(const char *fmt, ...);
int check_connection(const char *ifname);
void update_ip_config(caStaSetIpConfig_t *ipconfig);
void remove_wpa_networks(const char *ifname);
int find_network(const char *ifname, const char *ssid);
int add_network_common(const char *ifname);
int str_same(const char* str1, const char* str2);
int set_wpa_common(const char* intf, const int id);
int set_eap_common(const char* intf, const int id);
void reset_param();
int *get_param_val(const char* name);
char *get_param_str(const char* name);
void set_param_val(const char* name, int value);
void set_param_str(const char* name, char *value);
int sta_set_open(int *respLen, BYTE *respBuf);
int sta_set_owe(int *respLen, BYTE *respBuf);
int sta_mon_event(const char* ifname, const char* event);
int sta_scan_channel(const char* intf, int channel);
void setTxPPDUEnable(int);

int get_ip_config(const char* intf, caStaGetIpConfigResp_t *ifinfo);
int get_mac(const char* intf, unsigned char* addr);
void mbo_set_non_pref_chan(const char* intf, struct mbo_non_pref_channel* chans);
void mbo_set_cellular_data_cap(const char* intf, int cellularDataCap);
void fixed_rate_config(const char* intf, int isconnect);
static void init_driver_gen(void);

#ifdef _FREERTOS
int set_sigma_mode(int mode);
#endif

// AP functions
#ifdef CONFIG_MTK_AP
static void init_ap_params(void);
static void config_hostapd_htcap(FILE *f, ap_params_t *p_ap_params);
static void config_driver_wmm_params(FILE *f, ap_params_t *p_ap_params);
static void write_hostapd_conf();
static void write_driver_conf();
static int run_hostapd_cli(char *buf);
static void setup_ap_bridge_n_address();
static void mtk_add_ap_bridge();
static void mtk_del_ap_bridge();
static int is_interface_up(char *inf);
static int mtk_bridge_setup(const char *brname, enum ENUM_BRIDGE_OP_TYPE type);
static int mtk_bridge_intf_setup(const char *bridge, const char *dev,
        enum ENUM_BRIDGE_OP_TYPE type);
#endif

static char *get_main_intf(void);
static char *get_sta_intf(const char* ifname);
// P2P functions
#ifdef CONFIG_MTK_P2P
static void init_p2p_params(void);
static int p2p_is_peer_known(const char *ifname, const char *peer, int discovered);
static int p2p_find_peer(const char *ifname, const char *peer, int discovered);
static void enable_dhcp(char *ifname, int enable, int go);
static void *p2p_event_receive(void* data);
static void remove_p2p_persistent_networks(const char *ifname);
#endif

#ifdef CONFIG_MTK_WFD
extern int wfaMtkWfdCmd_rtspTeardown(void);
extern int wfaMtkWfdCmd_rtspPause(void);
extern int wfaMtkWfdCmd_rtspPlay(void);
extern int wfaMtkWfdCmd_rtspUibcCapUpdate(int type);
extern int wfaMtkWfdCmd_rtspEnterStandby(void);
static void *wfaStaWaitingWfdConnection_AutoGO(void* data);
static void *wfaStaWaitingWfdConnection_Nego(void* data);
extern int wfaMtkWfdCmd_rtspUibcGenEvent(int evtType);
extern int wfaMtkWfdCmd_rtspUibcHidcEvent(int hidType);
extern int wfaMtkWfdCmd_rtspSendIdrReq(void);
extern int wfaMtkWfdInit(void);
static int *mtkWfdP2pConnect(char *p2pPeerMac, char *sessionId, int sessionIdLen);
#endif


#define CERTIFICATES_PATH    "/etc/wpa_supplicant"

/* Some device may only support UDP ECHO, activate this line */
//#define WFA_PING_UDP_ECHO_ONLY 1

#define WFA_ENABLED 1

extern unsigned short wfa_defined_debug;
int wfaExecuteCLI(char *CLI);

/* Since the two definitions are used all over the CA function */
char gCmdStr[WFA_CMD_STR_SZ];
dutCmdResponse_t gGenericResp;
int wfaTGSetPrio(int sockfd, int tgClass);
void create_apts_msg(int msg, unsigned int txbuf[],int id);

int sret = 0;

extern char e2eResults[];

FILE *e2efp = NULL;

/* MTK INBAND Command & Data*/
#define MTK_INBAND_NONE             "MTK_INBAND_NONE"
#define MTK_INBAND_INIT             "MTK_INBAND_INIT"
#define MTK_INBAND_TXOP_ENHANCE     "MTK_INBAND_TXOP_ENHANCE"
#define MTK_INBAND_FIXED_RATE       "MTK_INBAND_FIXED_RATE"
#define MTK_INBAND_BASIZE_ADJUST    "MTK_INBAND_BASIZE_ADJUST"
#define MTK_INBAND_TXPOWER_ADJUST   "MTK_INBAND_TXPOWER_ADJUST"
#define MTK_INBAND_SET_FWLOG        "MTK_INBAND_SET_FWLOG"
#define MTK_INBAND_DISABLE_BACKGROUND_SCAN        "MTK_INBAND_DISABLE_BACKGROUND_SCAN"

int chk_ret_status()
{
    char *ret = getenv(WFA_RET_ENV);

    if(*ret == '1')
        return WFA_SUCCESS;
    else
        return WFA_FAILURE;
}

/*
 * agtCmdProcGetVersion(): response "ca_get_version" command to controller
 *  input:  cmd --- not used
 *          valLen -- not used
 *  output: parms -- a buffer to store the version info response.
 */
int agtCmdProcGetVersion(int len, BYTE *parms, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *getVerResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "\nEntering agtCmdProcGetVersion ...\n");

    getVerResp->status = STATUS_COMPLETE;
    wSTRNCPY(getVerResp->cmdru.version, MTK_SYSTEM_VER, sizeof(getVerResp->cmdru.version));
    getVerResp->cmdru.version[sizeof(getVerResp->cmdru.version) - 1] = '\0';

    wfaEncodeTLV(WFA_GET_VERSION_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)getVerResp, respBuf);

    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaAssociate():
 *    The function is to force the station wireless I/F to re/associate
 *    with the AP.
 */
int wfaStaAssociate(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *assoc = (dutCommand_t *)caCmdBuf;
    char *ifname = get_sta_intf(assoc->intf);
    dutCmdResponse_t *staAssocResp = &gGenericResp;
    char extra[50], buf[1024];

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaAssociate ...\n");

    if (strcmp(assoc->cmdsu.assoc.ssid, gDut.ssid) == 0) {
        DPRINT_INFO(WFA_OUT, "Sta associate for the most recently added network\n");
    } else if (find_network(ifname, assoc->cmdsu.assoc.ssid) < 0) {
        DPRINT_ERR(WFA_ERR, "Sta associate for a previously stored network profile but not found\n");
        staAssocResp->status = STATUS_ERROR;
        goto done;
    }

    if(assoc->cmdsu.assoc.bssid[0] != '\0') {
        if (set_network(ifname, gDut.networkId, "bssid", assoc->cmdsu.assoc.bssid) < 0) {
            staAssocResp->status = STATUS_ERROR;
            goto done;
        }
    }

    extra[0] = '\0';
    if (assoc->cmdsu.assoc.channel)
        wSPRINTF(extra, " freq=%u", channel2freq(assoc->cmdsu.assoc.channel));
    wSPRINTF(buf, "SELECT_NETWORK %d%s", gDut.networkId, extra);
    if (wpa_command(ifname, buf) < 0) {
        DPRINT_ERR(WFA_ERR, "Failed to select network id %d on %s",
                        gDut.networkId, ifname);
        staAssocResp->status = STATUS_ERROR;
        goto done;
    }
    wSPRINTF(buf, "ENABLE_NETWORK %d", gDut.networkId);
    if (wpa_command(ifname, buf) < 0) {
        DPRINT_ERR(WFA_ERR, "Failed to enable network id %d on %s",
                        gDut.networkId, ifname);
        staAssocResp->status = STATUS_ERROR;
        goto done;
    }

    sta_mon_event(ifname, "CTRL-EVENT-CONNECTED");
    update_ip_config(NULL);

    staAssocResp->status = STATUS_COMPLETE;
done:
    wfaEncodeTLV(WFA_STA_ASSOCIATE_RESP_TLV, 4, (BYTE *)staAssocResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaReAssociate():
 *    The function is to force the station wireless I/F to re/associate
 *    with the AP.
 */
int wfaStaReAssociate(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *assoc = (dutCommand_t *)caCmdBuf;
    char *ifname = assoc->intf;
    dutCmdResponse_t *staAssocResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaReAssociate ...\n");

    if(assoc->cmdsu.assoc.bssid[0] != '\0') {
        if (set_network(ifname, gDut.networkId, "bssid", assoc->cmdsu.assoc.bssid) < 0) {
            staAssocResp->status = STATUS_ERROR;
            goto done;
        }
    } else {
        staAssocResp->status = STATUS_INVALID;
        goto done;
    }

    sta_scan_channel(ifname, assoc->cmdsu.assoc.channel);

    if (wpa_command(ifname, "REASSOCIATE") < 0) {
        DPRINT_ERR(WFA_ERR, "Failed to reassociate network id %d on %s",
                        gDut.networkId, ifname);
        staAssocResp->status = STATUS_ERROR;
        goto done;
    }

    sta_mon_event(ifname, "CTRL-EVENT-CONNECTED");
    update_ip_config(NULL);

    /*
     * Then report back to control PC for completion.
     * This does not have failed/error status. The result only tells
     * a completion.
     */
    staAssocResp->status = STATUS_COMPLETE;
done:
    wfaEncodeTLV(WFA_STA_ASSOCIATE_RESP_TLV, 4, (BYTE *)staAssocResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaIsConnected():
 *    The function is to check whether the station's wireless I/F has
 *    already connected to an AP.
 */
int wfaStaIsConnected(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *connStat = (dutCommand_t *)caCmdBuf;
    dutCmdResponse_t *staConnectResp = &gGenericResp;
    char *ifname = get_sta_intf(connStat->intf);
    int *bw_sgnl = get_param_val("bw_sgnl");

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaIsConnected ...\n");

    staConnectResp->cmdru.connected = check_connection(ifname);

    fixed_rate_config(connStat->intf, staConnectResp->cmdru.connected);

#ifdef CONFIG_MTK_HE
    if (gDut.program == PROG_TYPE_HE) {
        shell(MTKINBANDCMD" %s %s", connStat->intf, MTK_INBAND_FIXED_RATE);
#ifdef _FREERTOS
        if (strcmp("HE-5.45.1", gDut.ssid) == 0 || strcmp("HE-5.45.2", gDut.ssid) == 0)
            shell(IWPRIV" %s driver \"fixedrate=%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d\"",
                connStat->intf,
                1,/*wlan-id*/
                8,/*Phy Mode*/
                0,/*BW*/
                7,/*Rate*/
                1,/*Nss*/
                1,/*GI*/
                0,/*Preamble*/
                0,/*STBC*/
                0,/*LDPC*/
                0,/*SPE_EN*/
                1,/*HE-LTF*/
                0,/*HE-ER-DCM*/
                0/*HE-ER-106*/);
        else if (strcmp("HE-5.61.1_24G", gDut.ssid) == 0 || strcmp("HE-5.61.1", gDut.ssid) == 0)
            gDut.trafficIteration = 0;
#endif
    }
#endif

    if (bw_sgnl && *bw_sgnl) {
        shell(IWPRIV" %s driver \"set_chip edcca 0 0\"", connStat->intf);
    }

    /*
     * Report back the status: Complete or Failed.
     */
    staConnectResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_IS_CONNECTED_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)staConnectResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaGetIpConfig():
 * This function is to retriev the ip info including
 *     1. dhcp enable
 *     2. ip address
 *     3. mask
 *     4. primary-dns
 *     5. secondary-dns
 *
 *     The current implementation is to use a script to find these information
 *     and store them in a file.
 */
int wfaStaGetIpConfig(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *getIpConf = (dutCommand_t *)caCmdBuf;
    dutCmdResponse_t *ipconfigResp = &gGenericResp;
    char *ifname = get_sta_intf(getIpConf->intf);
    caStaGetIpConfigResp_t *ifinfo = &ipconfigResp->cmdru.getIfconfig;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaGetIpConfig ...\n");

    if (get_ip_config(ifname, ifinfo) == WFA_SUCCESS) {
        ipconfigResp->status = STATUS_COMPLETE;
    } else {
        ipconfigResp->status = STATUS_ERROR;
    }
    ifinfo->isDhcp = gDut.sta_params.ipconfig.isDhcp;

    /*
     * report status
     */
    wfaEncodeTLV(WFA_STA_GET_IP_CONFIG_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)ipconfigResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaSetIpConfig():
 *   The function is to set the ip configuration to a wireless I/F.
 *   1. IP address
 *   2. Mac address
 *   3. default gateway
 *   4. dns nameserver (pri and sec).
 */
int wfaStaSetIpConfig(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *setIpConf = (dutCommand_t *)caCmdBuf;
    caStaSetIpConfig_t *ipconfig = &setIpConf->cmdsu.ipconfig;
    dutCmdResponse_t *staSetIpResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetIpConfig ... dhcp=%d\n", ipconfig->isDhcp);

    update_ip_config(ipconfig);

    /*
     * report status
     */
    staSetIpResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_IP_CONFIG_RESP_TLV, 4, (BYTE *)staSetIpResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaVerifyIpConnection():
 * The function is to verify if the station has IP connection with an AP by
 * send ICMP/pings to the AP.
 */
int wfaStaVerifyIpConnection(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *verip = (dutCommand_t *)caCmdBuf;
    dutCmdResponse_t *verifyIpResp = &gGenericResp;

#ifndef WFA_PING_UDP_ECHO_ONLY
#ifdef _FREERTOS
    /* execute the ping command  and pipe the result to a tmp file */
#if 0
    sprintf(gCmdStr, "ping %s 3", verip->cmdsu.verifyIp.dipaddr);
    sret = system(gCmdStr);
#else
    ping_request(3, verip->cmdsu.verifyIp.dipaddr, PING_IP_ADDR_V4, 64, NULL);
#endif
    /* Wait ping result */
    vTaskDelay(3000);
    verifyIpResp->status = STATUS_COMPLETE;
    if(g_ping_recv == 0)
        verifyIpResp->cmdru.connected = 0;
    else
        verifyIpResp->cmdru.connected = 1;
#else
    char strout[64], *pcnt;
    FILE *tmpfile;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaVerifyIpConnection ...\n");

    /* set timeout value in case not set */
    if(verip->cmdsu.verifyIp.timeout <= 0)
    {
        verip->cmdsu.verifyIp.timeout = 10;
    }

    /* execute the ping command  and pipe the result to a tmp file */
    sprintf(gCmdStr, "ping %s -c 3 -W %u | grep loss | cut -f3 -d, 1>& /tmp/pingout.txt", verip->cmdsu.verifyIp.dipaddr, verip->cmdsu.verifyIp.timeout);
    sret = system(gCmdStr);

    /* scan/check the output */
    tmpfile = fopen("/tmp/pingout.txt", "r+");
    if(tmpfile == NULL)
    {
        verifyIpResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_VERIFY_IP_CONNECTION_RESP_TLV, 4, (BYTE *)verifyIpResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;

        DPRINT_ERR(WFA_ERR, "file open failed\n");
        return WFA_FAILURE;
    }

    verifyIpResp->status = STATUS_COMPLETE;
    if(fscanf(tmpfile, "%63s", strout) == EOF)
        verifyIpResp->cmdru.connected = 0;
    else
    {
        pcnt = strtok(strout, "%");

        /* if the loss rate is 100%, not able to connect */
        if(atoi(pcnt) == 100)
            verifyIpResp->cmdru.connected = 0;
        else
            verifyIpResp->cmdru.connected = 1;
    }

    fclose(tmpfile);
#endif
#else
    int btSockfd;
    struct pollfd fds[2];
    int timeout = 2000;
    char anyBuf[64];
    struct sockaddr_in toAddr;
    int done = 1, cnt = 0, ret, nbytes;

    verifyIpResp->status = STATUS_COMPLETE;
    verifyIpResp->cmdru.connected = 0;

    btSockfd = wfaCreateUDPSock("127.0.0.1", WFA_UDP_ECHO_PORT);

    if(btSockfd == -1)
    {
        verifyIpResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_VERIFY_IP_CONNECTION_RESP_TLV, 4, (BYTE *)verifyIpResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;
        return WFA_FAILURE;;
    }

    toAddr.sin_family = AF_INET;
    toAddr.sin_addr.s_addr = inet_addr(verip->cmdsu.verifyIp.dipaddr);
    toAddr.sin_port = htons(WFA_UDP_ECHO_PORT);

    while(done)
    {
        wfaTrafficSendTo(btSockfd, (char *)anyBuf, 64, (struct sockaddr *)&toAddr);
        cnt++;

        fds[0].fd = btSockfd;
        fds[0].events = POLLIN | POLLOUT;

        ret = poll(fds, 1, timeout);
        switch(ret)
        {
        case 0:
            /* it is time out, count a packet lost*/
            break;
        case -1:
        /* it is an error */
        default:
        {
            switch(fds[0].revents)
            {
            case POLLIN:
            case POLLPRI:
            case POLLOUT:
                nbytes = wfaTrafficRecv(btSockfd, (char *)anyBuf, (struct sockaddr *)&toAddr, 64);
                if(nbytes != 0)
                    verifyIpResp->cmdru.connected = 1;
                done = 0;
                break;
            default:
                /* errors but not care */
                ;
            }
        }
        }
        if(cnt == 3)
        {
            done = 0;
        }
    }

#endif

    wfaEncodeTLV(WFA_STA_VERIFY_IP_CONNECTION_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)verifyIpResp, respBuf);

    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaGetMacAddress()
 *    This function is to retrieve the MAC address of a wireless I/F.
 */
int wfaStaGetMacAddress(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    unsigned char addr[6];
    dutCommand_t *getMac = (dutCommand_t *)caCmdBuf;
    dutCmdResponse_t *getmacResp = &gGenericResp;
    char *ifname = getMac->intf;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaGetMacAddress ...\n");
    memset(addr, 0x0, sizeof(addr));
    getmacResp->status = get_mac(ifname, addr);
    wSNPRINTF(getmacResp->cmdru.mac, sizeof(getmacResp->cmdru.mac),
            "%02x:%02x:%02x:%02x:%02x:%02x",
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    wfaEncodeTLV(WFA_STA_GET_MAC_ADDRESS_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)getmacResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaGetStats():
 * The function is to retrieve the statistics of the I/F's layer 2 txFrames,
 * rxFrames, txMulticast, rxMulticast, fcsErrors/crc, and txRetries.
 * Currently there is not definition how to use these info.
 */
int wfaStaGetStats(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *statsResp = &gGenericResp;

    /* this is never used, you can skip this call */

    statsResp->status = STATUS_ERROR;
    wfaEncodeTLV(WFA_STA_GET_STATS_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)statsResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);


    return WFA_SUCCESS;
}

int wfaSetEncryption(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEncryption_t *setEncryp = (caStaSetEncryption_t *)caCmdBuf;
    char *ifname = get_param_str("intf");
    dutCmdResponse_t *setEncrypResp = &gGenericResp;
    char buf[200];
    int id, i;
    int *activeKeyIdx;
    const char *keys[4], *encpType;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaSetEncryption ...\n");

    if (setEncryp) {
        set_param_str("ssid", setEncryp->ssid);
        set_param_str("encpType", setEncryp->encpType == ENCRYPT_WEP ? "wep" : "");
        set_param_str("key1", setEncryp->keys[0]);
        set_param_str("key2", setEncryp->keys[1]);
        set_param_str("key3", setEncryp->keys[2]);
        set_param_str("key4", setEncryp->keys[3]);
        set_param_val("activeKeyIdx", setEncryp->activeKeyIdx);
        ifname = setEncryp->intf;
    }

    id = add_network_common(ifname);
    if (id < 0) {
        setEncrypResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_network(ifname, id, "key_mgmt", "NONE") < 0) {
        setEncrypResp->status = STATUS_ERROR;
        goto done;
    }

    encpType = get_param_str("encpType");
    activeKeyIdx = get_param_val("activeKeyIdx");
    keys[0] = get_param_str("key1");
    keys[1] = get_param_str("key2");
    keys[2] = get_param_str("key3");
    keys[3] = get_param_str("key4");

    if (str_same(encpType, "wep")) {
        if (activeKeyIdx == NULL || *activeKeyIdx < 1 || *activeKeyIdx > 4) {
            setEncrypResp->status = STATUS_INVALID;
            goto done;
        }

        wSPRINTF(buf, "%d", *activeKeyIdx - 1);
        if (set_network(ifname, id, "wep_tx_keyidx", buf) < 0) {
            setEncrypResp->status = STATUS_ERROR;
            goto done;
        }

        for (i = 0; i < 4; i++) {
            if (keys[i] == NULL || keys[i][0] == '\0')
                continue;
            wSPRINTF(buf, "wep_key%d", i);
            if (set_network(ifname, id, buf, keys[i]) < 0) {
                setEncrypResp->status = STATUS_ERROR;
                goto done;
            }
        }
    } else {
        for(i = 0; i < 4; i++) {
            wSPRINTF(buf, "wep_key%d", i);
            set_network_quoted(ifname, id, buf, "");
        }
    }

    setEncrypResp->status = STATUS_COMPLETE;
done:
    wfaEncodeTLV(WFA_STA_SET_ENCRYPTION_RESP_TLV, 4, (BYTE *)setEncrypResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSetSecurity(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *cmd = (dutCommand_t *)caCmdBuf;
    char *intf = cmd->intf;
    caStaSetSecurity_t *ssec = &cmd->cmdsu.setsec;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetSecurity ...intf=%s, type=%d, pmf=%d\n", intf, ssec->type, ssec->pmf);
    if (intf[0] == '\0')
        return WFA_FAILURE;

    set_param_str("intf", intf);
    set_param_val("type", ssec->type);
    set_param_str("ssid", ssec->ssid);
    set_param_str("keyMgmtType", ssec->keyMgmtType);
    set_param_str("encpType", ssec->encpType);
    set_param_val("pmf", ssec->pmf);
    set_param_val("akmSuiteType", ssec->akmSuiteType);
    set_param_str("clientCertificate", ssec->clientCertificate);
    set_param_str("certType", ssec->certType);
    set_param_str("ecGroupID", ssec->ecGroupID);
    set_param_str("groupCipher", ssec->groupCipher);
    set_param_str("groupMgntCipher", ssec->groupMgntCipher);
    set_param_str("innerEAP", ssec->innerEAP);
    set_param_str("invalidSAEElement", ssec->invalidSAEElement);
    set_param_str("networkMode", ssec->networkMode);
    set_param_val("owe", ssec->owe);
    set_param_str("pacFile", ssec->pacFile);
    set_param_str("pairwiseCipher", ssec->pairwiseCipher);
    set_param_str("passphrase", ssec->passphrase);
    set_param_str("password", ssec->passphrase);
    set_param_val("pmksaCaching", ssec->pmksaCaching);
    set_param_val("profile", ssec->profile);
    set_param_str("prog", ssec->prog);
    set_param_str("trustedRootCA", ssec->trustedRootCA);
    set_param_str("tlsCipher", ssec->tlsCipher);
    set_param_str("username", ssec->username);
    set_param_val("peapVersion", ssec->peapVersion);
    set_param_str("key1", ssec->keys[0]);
    set_param_str("key2", ssec->keys[1]);
    set_param_str("key3", ssec->keys[2]);
    set_param_str("key4", ssec->keys[3]);
    set_param_val("activeKeyIdx", ssec->activeKeyIdx);
    set_param_val("saePwe", ssec->saePwe);

    if (ssec->type == SEC_TYPE_PSK ||
        ssec->type == SEC_TYPE_PSK_SAE ||
        ssec->type == SEC_TYPE_SAE)
        return wfaStaSetPSK(0, NULL, respLen, respBuf);
    if (ssec->type == SEC_TYPE_EAPTLS)
        return wfaStaSetEapTLS(0, NULL, respLen, respBuf);
    if (ssec->type == SEC_TYPE_EAPTTLS)
        return wfaStaSetEapTTLS(0, NULL, respLen, respBuf);
    if (ssec->type == SEC_TYPE_EAPPEAP)
        return wfaStaSetPEAP(0, NULL, respLen, respBuf);
    if (ssec->type == SEC_TYPE_EAPSIM)
        return wfaStaSetEapSIM(0, NULL, respLen, respBuf);
    if (ssec->type == SEC_TYPE_EAPAKA)
        return wfaStaSetEapAKA(0, NULL, respLen, respBuf);
    if (ssec->type == SEC_TYPE_OPEN)
        return sta_set_open(respLen, respBuf);
    if (ssec->type == SEC_TYPE_OWE)
        return sta_set_owe(respLen, respBuf);
    if (ssec->type == SEC_TYPE_WEP)
        return wfaSetEncryption(0, NULL, respLen, respBuf);
    return WFA_FAILURE;
}

/*
 * wfaStaSetEapTLS():
 *   This is to set
 *   1. ssid
 *   2. encrypType - tkip or aes-ccmp
 *   3. keyManagementType - wpa or wpa2
 *   4. trustedRootCA
 *   5. clientCertificate
 */
int wfaStaSetEapTLS(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEapTLS_t *setTLS = (caStaSetEapTLS_t *)caCmdBuf;
    char *intf = get_param_str("intf");
    dutCmdResponse_t *setEapTlsResp = &gGenericResp;
    int id;

    if (setTLS) {
        DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetEapTLS ...\n");
        set_param_str("ssid", setTLS->ssid);
        set_param_str("username", setTLS->username);
        set_param_str("keyMgmtType", setTLS->keyMgmtType);
        set_param_str("encptype", setTLS->encrptype);
        set_param_str("trustedRootCA", setTLS->trustedRootCA);
        set_param_str("clientCertificate", setTLS->clientCertificate);
        set_param_val("pmf", setTLS->pmf);
        set_param_str("micAlg", setTLS->micAlg);
        intf = setTLS->intf;
    }

    id = add_network_common(intf);
    if (id < 0) {
        setEapTlsResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_eap_common(intf, id) != STATUS_COMPLETE) {
        setEapTlsResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_network(intf, id, "eap", "TLS") < 0) {
        setEapTlsResp->status = STATUS_ERROR;
        goto done;
    }

    if (!get_param_str("username") &&
        set_network_quoted(intf, id, "identity", "wifi-user@wifilabs.local") < 0) {
            setEapTlsResp->status = STATUS_ERROR;
            goto done;
    }

    if (set_network_quoted(intf, id, "private_key_passwd", "wifi") < 0) {
        setEapTlsResp->status = STATUS_ERROR;
        goto done;
    }

    setEapTlsResp->status = STATUS_COMPLETE;
done:
    wfaEncodeTLV(WFA_STA_SET_EAPTLS_RESP_TLV, 4, (BYTE *)setEapTlsResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * The function is to set
 *   1. ssid
 *   2. passPhrase
 *   3. keyMangementType - wpa/wpa2
 *   4. encrypType - tkip or aes-ccmp
 */
int wfaStaSetPSK(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetPSK_t *setPSK = (caStaSetPSK_t *)caCmdBuf;
    char *intf = get_param_str("intf");
    dutCmdResponse_t *setPskResp = &gGenericResp;
    const char *val, *alg;
    const int *type, *pmf, *pwe;
    char buf[256];
    int id;

    if (setPSK) {
        DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetPSK ...ssid=%s,psk=%s\n", setPSK->ssid, setPSK->passphrase);
        set_param_str("ssid", setPSK->ssid);
        set_param_str("passphrase", (char *)setPSK->passphrase);
        set_param_str("keyMgmtType", setPSK->keyMgmtType);
        set_param_str("micAlg", setPSK->micAlg);
        set_param_str("prog", setPSK->prog);
        set_param_val("prefer", setPSK->prefer);
        set_param_str("encpType", encpType2str(setPSK->encpType));
        set_param_val("pmf", setPSK->pmf);
        intf = get_sta_intf(setPSK->intf);
    }

    id = add_network_common(intf);
    if (id < 0)
        return STATUS_ERROR;

    if (set_wpa_common(intf, id) != STATUS_COMPLETE)
        return STATUS_ERROR;

    buf[0] = '\0';

    type = get_param_val("type");
    alg = get_param_str("micAlg");
    pmf = get_param_val("pmf");
    val = get_param_str("keyMgmtType");
    // key_mgmt
    if (type && *type == SEC_TYPE_SAE) {
        if (str_same(val, "wpa2-ft"))
            wSPRINTF(buf, "FT-SAE");
        else
            wSPRINTF(buf, "SAE");
        if (wpa_command(intf, "SET sae_groups 19 20 21") < 0) {
            setPskResp->status = STATUS_ERROR;
            goto done;
        }
        if (!pmf) {
            gDut.sta_pmf = WFA_REQUIRED;
            if (set_network(intf, id, "ieee80211w", "2") < 0) {
                setPskResp->status = STATUS_ERROR;
                goto done;
            }
        }
    } else if (type && *type == SEC_TYPE_PSK_SAE) {
        if (str_same(val, "wpa2-ft"))
            wSPRINTF(buf, "FT-SAE FT-PSK");
        else
            wSPRINTF(buf, "SAE WPA-PSK");
        if (wpa_command(intf, "SET sae_groups 19 20 21") < 0) {
            setPskResp->status = STATUS_ERROR;
            goto done;
        }
        if (!pmf) {
            gDut.sta_pmf = WFA_OPTIONAL;
            if (set_network(intf, id, "ieee80211w", "1") < 0) {
                setPskResp->status = STATUS_ERROR;
                goto done;
            }
        }
    } else if (str_same(alg, "SHA-256")) {
        wSPRINTF(buf, "WPA-PSK-SHA256");
    } else if (str_same(alg, "SHA-1")) {
        wSPRINTF(buf, "WPA-PSK");
    } else if (str_same(val, "wpa2-ft")) {
        wSPRINTF(buf, "FT-PSK");
    } else if (str_same(val, "wpa2-sha256") ||
        gDut.sta_pmf == WFA_REQUIRED ||
        gDut.sta_pmf == WFA_OPTIONAL) {
        wSPRINTF(buf, "WPA-PSK WPA-PSK-SHA256");
    } else {
        wSPRINTF(buf, "WPA-PSK");
    }

    if (set_network(intf, id, "key_mgmt", buf) < 0) {
        setPskResp->status = STATUS_ERROR;
        goto done;
    }

    // passphrase
    val = get_param_str("passPhrase");
    if (type && *type == SEC_TYPE_SAE) {
#if (WPA_SUPPLICANT_VER == WPA_SUPPLICANT_2_6)
        if (set_network_quoted(intf, id, "psk", val) < 0) {
            setPskResp->status = STATUS_ERROR;
            goto done;
#else
        if (set_network_quoted(intf, id, "sae_password", val) < 0) {
            setPskResp->status = STATUS_ERROR;
            goto done;
#endif
        }
    } else {
        if (set_network_quoted(intf, id, "psk", val) < 0) {
            setPskResp->status = STATUS_ERROR;
            goto done;
        }
    }

    // sae_groups
    val = get_param_str("ECGroupID");
    if (val) {
        wSPRINTF(buf, "SET sae_groups %s", val);
        if (wpa_command(intf, buf) < 0) {
            setPskResp->status = STATUS_ERROR;
            goto done;
        }
    }

    val = get_param_str("InvalidSAEElement");
    if (val) {
        wSPRINTF(buf, "SET sae_commit_override %s", val);
        if (wpa_command(intf, buf) < 0) {
            setPskResp->status = STATUS_ERROR;
            goto done;
        }
    }

    pwe = get_param_val("saePwe");
    if (pwe) {
            wSPRINTF(buf, "SET sae_pwe %d", *pwe);
            if (wpa_command(intf, buf) < 0) {
                setPskResp->status = STATUS_ERROR;
                goto done;
            }
    }

    setPskResp->status = STATUS_COMPLETE;
done:
    wfaEncodeTLV(WFA_STA_SET_PSK_RESP_TLV, 4, (BYTE *)setPskResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaGetInfo():
 * Get vendor specific information in name/value pair by a wireless I/F.
 */
int wfaStaGetInfo(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    dutCommand_t *getInfo = (dutCommand_t *)caCmdBuf;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaGetInfo ...\n");

    /*
     * Normally this is called to retrieve the vendor information
     * from a interface, no implement yet
     */
    wSPRINTF(infoResp.cmdru.info, "interface,%s,vendor,Mediatek Inc.,cardtype,802.11a/b/g/n/ac", getInfo->intf);

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_GET_INFO_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaSetEapTTLS():
 *   This is to set
 *   1. ssid
 *   2. username
 *   3. passwd
 *   4. encrypType - tkip or aes-ccmp
 *   5. keyManagementType - wpa or wpa2
 *   6. trustedRootCA
 */
int wfaStaSetEapTTLS(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEapTTLS_t *setTTLS = (caStaSetEapTTLS_t *)caCmdBuf;
    char *intf = get_param_str("intf");
    dutCmdResponse_t *setEapTtlsResp = &gGenericResp;
    int id;

    if (setTTLS) {
        DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetEapTTLS ...\n");
        set_param_str("ssid", setTTLS->ssid);
        set_param_str("username", setTTLS->username);
        set_param_str("password", setTTLS->passwd);
        set_param_str("keyMgmtType", setTTLS->keyMgmtType);
        set_param_str("encptype", setTTLS->encrptype);
        set_param_str("trustedRootCA", setTTLS->trustedRootCA);
        set_param_str("clientCertificate", setTTLS->clientCertificate);
        set_param_val("pmf", setTTLS->pmf);
        set_param_str("micAlg", setTTLS->micAlg);
        set_param_str("prog", setTTLS->prog);
        set_param_val("prefer", setTTLS->prefer);
        intf = setTTLS->intf;
    }

    id = add_network_common(intf);
    if (id < 0) {
        setEapTtlsResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_eap_common(intf, id) != STATUS_COMPLETE) {
        setEapTtlsResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_network(intf, id, "eap", "TTLS") < 0) {
        setEapTtlsResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_network_quoted(intf, id, "phase2", "auth=MSCHAPV2") < 0) {
        setEapTtlsResp->status = STATUS_ERROR;
        goto done;
    }

    setEapTtlsResp->status = STATUS_COMPLETE;
done:
    wfaEncodeTLV(WFA_STA_SET_EAPTTLS_RESP_TLV, 4, (BYTE *)setEapTtlsResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaSetEapSIM():
 *   This is to set
 *   1. ssid
 *   2. user name
 *   3. passwd
 *   4. encrypType - tkip or aes-ccmp
 *   5. keyMangementType - wpa or wpa2
 */
int wfaStaSetEapSIM(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEapSIM_t *setSIM = (caStaSetEapSIM_t *)caCmdBuf;
    char *intf = get_param_str("intf");
    dutCmdResponse_t *setEapSimResp = &gGenericResp;
    int id;

    if (setSIM) {
        DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetEapSIM ...\n");

        set_param_str("ssid", setSIM->ssid);
        set_param_str("username", setSIM->username);
        set_param_str("password", setSIM->passwd);
        set_param_str("keyMgmtType", setSIM->keyMgmtType);
        set_param_str("encptype", setSIM->encrptype);
        set_param_val("pmf", setSIM->pmf);
        intf = setSIM->intf;
    }

    id = add_network_common(intf);
    if (id < 0) {
        setEapSimResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_eap_common(intf, id) != STATUS_COMPLETE) {
        setEapSimResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_network(intf, id, "eap", "SIM") < 0) {
        setEapSimResp->status = STATUS_ERROR;
        goto done;

    }

    setEapSimResp->status = STATUS_COMPLETE;
done:
    wfaEncodeTLV(WFA_STA_SET_EAPSIM_RESP_TLV, 4, (BYTE *)setEapSimResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaSetPEAP()
 *   This is to set
 *   1. ssid
 *   2. user name
 *   3. passwd
 *   4. encryType - tkip or aes-ccmp
 *   5. keyMgmtType - wpa or wpa2
 *   6. trustedRootCA
 *   7. innerEAP
 *   8. peapVersion
 */
int wfaStaSetPEAP(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEapPEAP_t *setPEAP = (caStaSetEapPEAP_t *)caCmdBuf;
    char *intf = get_param_str("intf");
    dutCmdResponse_t *setPeapResp = &gGenericResp;
    int id;
    char buf[256];
    const char *val;

    if (setPEAP) {
        DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetPEAP ...\n");
        set_param_str("ssid", setPEAP->ssid);
        set_param_str("username", setPEAP->username);
        set_param_str("password", setPEAP->passwd);
        set_param_str("keyMgmtType", setPEAP->keyMgmtType);
        set_param_str("encptype", setPEAP->encrptype);
        set_param_str("trustedRootCA", setPEAP->trustedRootCA);
        set_param_str("innerEAP", setPEAP->innerEAP);
        set_param_val("peapVersion", setPEAP->peapVersion);
        set_param_val("pmf", setPEAP->pmf);
        intf = setPEAP->intf;
    }

    id = add_network_common(intf);
    if (id < 0) {
        setPeapResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_eap_common(intf, id) != STATUS_COMPLETE) {
        setPeapResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_network(intf, id, "eap", "PEAP") < 0) {
        setPeapResp->status = STATUS_ERROR;
        goto done;
    }

    val = get_param_str("innerEAP");
    if (val) {
        if (str_same(val, "MSCHAPv2")) {
            wSPRINTF(buf, "auth=MSCHAPV2");
        } else if (str_same(val, "GTC")) {
            wSPRINTF(buf, "auth=GTC");
        } else {
            setPeapResp->status = STATUS_INVALID;
            goto done;
        }
        if (set_network_quoted(intf, id, "phase2", buf) < 0) {
            setPeapResp->status = STATUS_ERROR;
            goto done;
        }
    }

    val = (const char *)get_param_val("peapVersion");
    if (val) {
        if (*val < 0 || *val > 1) {
            setPeapResp->status = STATUS_INVALID;
            goto done;
        }
        wSPRINTF(buf, "peapver=%d", *val);
        if (set_network_quoted(intf, id, "phase1", buf) < 0) {
            setPeapResp->status = STATUS_ERROR;
            goto done;
        }
    }

    setPeapResp->status = STATUS_COMPLETE;
done:
    wfaEncodeTLV(WFA_STA_SET_PEAP_RESP_TLV, 4, (BYTE *)setPeapResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaSetUAPSD()
 *    This is to set
 *    1. acBE
 *    2. acBK
 *    3. acVI
 *    4. acVO
 */
int wfaStaSetUAPSD(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *setUAPSDResp = &gGenericResp;
    caStaSetUAPSD_t *setuapsd = (caStaSetUAPSD_t *)caCmdBuf;
    char buf[256];
    unsigned int uapsd = 0;

    if (setuapsd->acBE)
            uapsd |= 1;
    if (setuapsd->acBK)
        uapsd |= 1<<1;
    if (setuapsd->acVI)
        uapsd |= 1<<2;
    if (setuapsd->acVO)
        uapsd |= 1<<3;
    if (wSTRNCMP(setuapsd->intf, "p2p", 3) == 0) {
        uapsd |= 0x10000;
    }

    shell(IWPRIV" %s set_sw_ctrl 0x10010003 0x%x", setuapsd->intf, uapsd);

    wSPRINTF(buf, "set uapsd %d,%d,%d,%d,%d", setuapsd->acBE, setuapsd->acBK,
        setuapsd->acVI, setuapsd->acVO, setuapsd->maxSPLength);
    wpa_command(setuapsd->intf, buf);

    setUAPSDResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_UAPSD_RESP_TLV, 4, (BYTE *)setUAPSDResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
    return WFA_SUCCESS;
}

int wfaDeviceGetInfo(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *dutCmd = (dutCommand_t *)caCmdBuf;
    caDevInfo_t *devInfo = &dutCmd->cmdsu.dev;
    dutCmdResponse_t *infoResp = &gGenericResp;
#if defined(_FREERTOS)
    caDeviceGetInfoResp_t dinfo = {"MediaTek Inc.", "MT7933", "MT7933", "MT7933"};
#else
    caDeviceGetInfoResp_t dinfo = {"MediaTek Inc.", "MT66xx", MTK_SYSTEM_VER};
#endif

    DPRINT_INFO(WFA_OUT, "\nEntering wfaDeviceGetInfo ... mode=%d\n", devInfo->fw);

    if(devInfo->fw == 0) {
#if defined(_FREERTOS)
	DPRINT_INFO(WFA_OUT, "\ndevice info: %s, %s, %s\n", dinfo.vendor, dinfo.model, dinfo.version);
#else
        if (chk_exist(BIN_DIR"/model.txt") &&
            shell("cat "BIN_DIR"/model.txt | sed s/[[:space:]]//g") == WFA_SUCCESS) {
            wSTRNCPY(dinfo.model, gDut.shellResult, sizeof(dinfo.model) - 1);
        } else if (shell("getprop ro.build.flavor") == WFA_SUCCESS) {
            wSTRNCPY(dinfo.model, gDut.shellResult, sizeof(dinfo.model) - 1);
        }
        if (shell("getprop ro.mediatek.version.release") == WFA_SUCCESS)
            wSTRNCPY(dinfo.version, gDut.shellResult, sizeof(dinfo.version) - 1);
#endif
    } else {
#if defined(_FREERTOS)
	DPRINT_INFO(WFA_OUT, "\ndevice info: %s, %s, %s, %s\n", dinfo.vendor, dinfo.model, dinfo.version, dinfo.firmware);
#else
        int size = sizeof(infoResp->cmdru.devInfo.firmware) - 1;
        if (shell("getprop persist.vendor.connsys.patch.version") == WFA_SUCCESS)
            wSTRNCPY(dinfo.firmware, gDut.shellResult, size);
        else
            wSTRNCPY(dinfo.firmware, "NOVERSION", size);

        /* TODO: should we parse fw version from iwpriv ?
        wSTRNCPY(infoResp->cmdru.devInfo.firmware, "NOVERSION", size);
        // get firmware version
        if (shell(IWPRIV" wlan0 driver ver") == WFA_SUCCESS) {
            char *outer = NULL, *outer_save, *inner = NULL, *inner_save = NULL;

            outer = strtok_r(gDut.shellResult, "\n\r", &outer_save);
            while (outer) {
                inner = strtok_r(outer, " ", &inner_save);
                if (inner && !wSTRNCMP(inner, "Tailer", 6)) {
                    wSTRNCPY(infoResp->cmdru.devInfo.firmware, inner_save, size);
                    break;
                }
                outer = strtok_r(NULL, "\n\r", &outer_save);
            }
        }
        */
#endif
    }

    wMEMCPY(&infoResp->cmdru.devInfo, &dinfo, sizeof(caDeviceGetInfoResp_t));

    /*
     * report status
     */
    infoResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_DEVICE_GET_INFO_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * This funciton is to retrieve a list of interfaces and return
 * the list back to Agent control.
 * ********************************************************************
 * Note: We intend to make this WLAN interface name as a hardcode name.
 * Therefore, for a particular device, you should know and change the name
 * for that device while doing porting. The MACRO "WFA_STAUT_IF" is defined in
 * the file "inc/wfa_ca.h". If the device OS is not linux-like, this most
 * likely is hardcoded just for CAPI command responses.
 * *******************************************************************
 *
 */
int wfaDeviceListIF(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    dutCommand_t *ifList = (dutCommand_t *)caCmdBuf;
    caDeviceListIFResp_t *ifListResp = &infoResp->cmdru.ifList;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaDeviceListIF ...\n");

    switch(ifList->cmdsu.iftype)
    {
    case IF_80211:
        infoResp->status = STATUS_COMPLETE;
        ifListResp->iftype = IF_80211;
        strcpy(ifListResp->ifs[0], get_main_intf());
        strcpy(ifListResp->ifs[1], "NULL");
        strcpy(ifListResp->ifs[2], "NULL");
        break;
    case IF_ETH:
        infoResp->status = STATUS_COMPLETE;
        ifListResp->iftype = IF_ETH;
        strcpy(ifListResp->ifs[0], "eth0");
        strcpy(ifListResp->ifs[1], "NULL");
        strcpy(ifListResp->ifs[2], "NULL");
        break;
    default:
    {
        infoResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_DEVICE_LIST_IF_RESP_TLV, 4, (BYTE *)infoResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;

        return WFA_SUCCESS;
    }
    }

    wfaEncodeTLV(WFA_DEVICE_LIST_IF_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaStaDebugSet(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *debugResp = &gGenericResp;
    dutCommand_t *debugSet = (dutCommand_t *)caCmdBuf;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaDebugSet ...\n");

    if(debugSet->cmdsu.dbg.state == 1) /* enable */
        wfa_defined_debug |= debugSet->cmdsu.dbg.level;
    else
        wfa_defined_debug = (~debugSet->cmdsu.dbg.level & wfa_defined_debug);

    debugResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_GET_INFO_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)debugResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);


    return WFA_SUCCESS;
}


/*
 *   wfaStaGetBSSID():
 *     This function is to retrieve BSSID of a specific wireless I/F.
 */
int wfaStaGetBSSID(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *dutCmd = (dutCommand_t *)caCmdBuf;
    dutCmdResponse_t *bssidResp = &gGenericResp;
    char *ifname = get_sta_intf(dutCmd->intf);;
    char bssid[20];
    int ret;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaGetBSSID ...\n");

    ret = get_wpa_status(ifname, "bssid", bssid, sizeof(bssid));
    if (ret >= 0) {
        DPRINT_INFO(WFA_OUT, "get_wpa_status bssid=%s\n", bssid);
        wSTRNCPY(bssidResp->cmdru.bssid, bssid, WFA_MAC_ADDR_STR_LEN);
    } else {
        DPRINT_INFO(WFA_OUT, "get_wpa_status field=\"bssid\" ret=%d\n", ret);
        wSTRNCPY(bssidResp->cmdru.bssid, "00:00:00:00:00:00", WFA_MAC_ADDR_STR_LEN);
    }

    bssidResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_GET_BSSID_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)bssidResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaSetIBSS()
 *    This is to set
 *    1. ssid
 *    2. channel
 *    3. encrypType - none or wep
 *    optional
 *    4. key1
 *    5. key2
 *    6. key3
 *    7. key4
 *    8. activeIndex - 1, 2, 3, or 4
 */
int wfaStaSetIBSS(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetIBSS_t *setIBSS = (caStaSetIBSS_t *)caCmdBuf;
    dutCmdResponse_t *setIbssResp = &gGenericResp;
    int i;

    /*
     * disable the network first
     */
    sprintf(gCmdStr, "wpa_cli -i %s disable_network 0", setIBSS->intf);
    sret = system(gCmdStr);

    /*
     * set SSID
     */
    sprintf(gCmdStr, "wpa_cli -i %s set_network 0 ssid '\"%s\"'", setIBSS->intf, setIBSS->ssid);
    sret = system(gCmdStr);

    /*
     * Set channel for IBSS
     */
    sprintf(gCmdStr, "iwconfig %s channel %i", setIBSS->intf, setIBSS->channel);
    sret = system(gCmdStr);

    /*
     * Tell the supplicant for IBSS mode (1)
     */
    sprintf(gCmdStr, "wpa_cli -i %s set_network 0 mode 1", setIBSS->intf);
    sret = system(gCmdStr);

    /*
     * set Key management to NONE (NO WPA) for plaintext or WEP
     */
    sprintf(gCmdStr, "wpa_cli -i %s set_network 0 key_mgmt NONE", setIBSS->intf);
    sret = system(gCmdStr);

    if(setIBSS->encpType == 1)
    {
        for(i=0; i<4; i++)
        {
            if(strlen(setIBSS->keys[i]) ==5 || strlen(setIBSS->keys[i]) == 13)
            {
                sprintf(gCmdStr, "wpa_cli -i %s set_network 0 wep_key%i \"%s\"",
                        setIBSS->intf, i, setIBSS->keys[i]);
                sret = system(gCmdStr);
            }
        }

        i = setIBSS->activeKeyIdx;
        if(strlen(setIBSS->keys[i]) ==5 || strlen(setIBSS->keys[i]) == 13)
        {
            sprintf(gCmdStr, "wpa_cli -i %s set_network 0 wep_tx_keyidx %i",
                    setIBSS->intf, setIBSS->activeKeyIdx);
            sret = system(gCmdStr);
        }
    }

    sprintf(gCmdStr, "wpa_cli -i %s enable_network 0", setIBSS->intf);
    sret = system(gCmdStr);

    setIbssResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_IBSS_RESP_TLV, 4, (BYTE *)setIbssResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 *  wfaSetMode():
 *  The function is to set the wireless interface with a given mode (possible
 *  adhoc)
 *  Input parameters:
 *    1. I/F
 *    2. ssid
 *    3. mode adhoc or managed
 *    4. encType
 *    5. channel
 *    6. key(s)
 *    7. active  key
 */
int wfaStaSetMode(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetMode_t *setmode = (caStaSetMode_t *)caCmdBuf;
    dutCmdResponse_t *SetModeResp = &gGenericResp;
    int i;

    /*
     * bring down the interface
     */
    sprintf(gCmdStr, "ifconfig %s down",setmode->intf);
    sret = system(gCmdStr);

    /*
     * distroy the interface
     */
    sprintf(gCmdStr, "wlanconfig %s destroy",setmode->intf);
    sret = system(gCmdStr);


    /*
     * re-create the interface with the given mode
     */
    if(setmode->mode == 1)
        sprintf(gCmdStr, "wlanconfig %s create wlandev wifi0 wlanmode adhoc",setmode->intf);
    else
        sprintf(gCmdStr, "wlanconfig %s create wlandev wifi0 wlanmode managed",setmode->intf);

    sret = system(gCmdStr);
    if(setmode->encpType == ENCRYPT_WEP)
    {
        int j = setmode->activeKeyIdx;
        for(i=0; i<4; i++)
        {
            if(setmode->keys[i][0] != '\0')
            {
                sprintf(gCmdStr, "iwconfig  %s key  s:%s",
                        setmode->intf, setmode->keys[i]);
                sret = system(gCmdStr);
            }
            /* set active key */
            if(setmode->keys[j][0] != '\0')
                sprintf(gCmdStr, "iwconfig  %s key  s:%s",
                        setmode->intf, setmode->keys[j]);
            sret = system(gCmdStr);
        }

    }
    /*
     * Set channel for IBSS
     */
    if(setmode->channel)
    {
        sprintf(gCmdStr, "iwconfig %s channel %i", setmode->intf, setmode->channel);
        sret = system(gCmdStr);
    }


    /*
     * set SSID
     */
    sprintf(gCmdStr, "iwconfig %s essid %s", setmode->intf, setmode->ssid);
    sret = system(gCmdStr);

    /*
     * bring up the interface
     */
    sprintf(gCmdStr, "ifconfig %s up",setmode->intf);
    sret = system(gCmdStr);

    SetModeResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_MODE_RESP_TLV, 4, (BYTE *)SetModeResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSetPwrSave(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetPwrSave_t *setps = (caStaSetPwrSave_t *)caCmdBuf;
    dutCmdResponse_t *SetPSResp = &gGenericResp;

    if (str_same(setps->mode, "off"))
        shell(IWPRIV" %s set_power_mode 0", setps->intf);
    else
        shell(IWPRIV" %s set_power_mode 1", setps->intf);

    SetPSResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_PWRSAVE_RESP_TLV, 4, (BYTE *)SetPSResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaUpload(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaUpload_t *upload = &((dutCommand_t *)caCmdBuf)->cmdsu.upload;
    dutCmdResponse_t *upLoadResp = &gGenericResp;
    caStaUploadResp_t *upld = &upLoadResp->cmdru.uld;

    if(upload->type == WFA_UPLOAD_VHSO_RPT)
    {
        int rbytes;
        /*
         * if asked for the first packet, always to open the file
         */
        if(upload->next == 1)
        {
            if(e2efp != NULL)
            {
                fclose(e2efp);
                e2efp = NULL;
            }

            e2efp = fopen(e2eResults, "r");
        }

        if(e2efp == NULL)
        {
            upLoadResp->status = STATUS_ERROR;
            wfaEncodeTLV(WFA_STA_UPLOAD_RESP_TLV, 4, (BYTE *)upLoadResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + 4;
            return WFA_FAILURE;
        }

        rbytes = fread(upld->bytes, 1, 256, e2efp);

        if(rbytes < 256)
        {
            /*
             * this means no more bytes after this read
             */
            upld->seqnum = 0;
            fclose(e2efp);
            e2efp=NULL;
        }
        else
        {
            upld->seqnum = upload->next;
        }

        upld->nbytes = rbytes;

        upLoadResp->status = STATUS_COMPLETE;
        wfaEncodeTLV(WFA_STA_UPLOAD_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)upLoadResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
    }
    else
    {
        upLoadResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_UPLOAD_RESP_TLV, 4, (BYTE *)upLoadResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;
    }

    return WFA_SUCCESS;
}
/*
 * wfaStaSetWMM()
 *  TO be ported on a specific plaform for the DUT
 *  This is to set the WMM related parameters at the DUT.
 *  Currently the function is used for GROUPS WMM-AC and WMM general configuration for setting RTS Threshhold, Fragmentation threshold and wmm (ON/OFF)
 *  It is expected that this function will set all the WMM related parametrs for a particular GROUP .
 */
int wfaStaSetWMM(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
#ifdef WFA_WMM_AC
    caStaSetWMM_t *setwmm = (caStaSetWMM_t *)caCmdBuf;
    char *ifname = setwmm->intf;
    dutCmdResponse_t *setwmmResp = &gGenericResp;
    char buf[1024];

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetWMM ...\n");

    switch(setwmm->group)
    {
    case GROUP_WMMAC:
        if (setwmm->send_trig)
        {
            int Sockfd, r;
            struct sockaddr_in psToAddr;
            unsigned int TxMsg[512];

            Sockfd = wfaCreateUDPSock(setwmm->dipaddr, 12346);
            if(Sockfd == -1) {
                setwmmResp->status = STATUS_ERROR;
                break;
            }

            memset(&psToAddr, 0, sizeof(psToAddr));
            psToAddr.sin_family = AF_INET;
            psToAddr.sin_addr.s_addr = inet_addr(setwmm->dipaddr);
            psToAddr.sin_port = htons(12346);


            switch (setwmm->trig_ac)
            {
            case WMMAC_AC_VO:
                wfaTGSetPrio(Sockfd, 7);
                create_apts_msg(APTS_CK_VO, TxMsg, 0);
                printf("\r\nSending AC_VO trigger packet\n");
                break;

            case WMMAC_AC_VI:
                wfaTGSetPrio(Sockfd, 5);
                create_apts_msg(APTS_CK_VI, TxMsg, 0);
                printf("\r\nSending AC_VI trigger packet\n");
                break;

            case WMMAC_AC_BK:
                wfaTGSetPrio(Sockfd, 2);
                create_apts_msg(APTS_CK_BK, TxMsg, 0);
                printf("\r\nSending AC_BK trigger packet\n");
                break;

            default:
            case WMMAC_AC_BE:
                wfaTGSetPrio(Sockfd, 0);
                create_apts_msg(APTS_CK_BE, TxMsg, 0);
                printf("\r\nSending AC_BE trigger packet\n");
                break;
            }

            r = sendto(Sockfd, TxMsg, 256, 0, (struct sockaddr *)&psToAddr,
                   sizeof(struct sockaddr));
            if(r < 0)
                DPRINT_INFO(WFA_OUT, "WMMAC sendto error %d\n", r);
            close(Sockfd);
            usleep(1000000);
        }
        else if (setwmm->action == WMMAC_ADDTS)
        {
            printf("ADDTS AC PARAMS: dialog id: %d, TID: %d, "
                   "DIRECTION: %d, PSB: %d, UP: %d, INFOACK: %d BURST SIZE DEF: %d"
                   "Fixed %d, MSDU Size: %d, Max MSDU Size %d, "
                   "MIN SERVICE INTERVAL: %d, MAX SERVICE INTERVAL: %d, "
                   "INACTIVITY: %d, SUSPENSION %d, SERVICE START TIME: %d, "
                   "MIN DATARATE: %d, MEAN DATA RATE: %d, PEAK DATA RATE: %d, "
                   "BURSTSIZE or MSDU Aggreg: %d, DELAY BOUND: %d, PHYRATE: %d, SPLUSBW: %f, "
                   "MEDIUM TIME: %d, ACCESSCAT: %d\n",
                   setwmm->actions.addts.dialog_token,
                   setwmm->actions.addts.tspec.tsinfo.TID,
                   setwmm->actions.addts.tspec.tsinfo.direction,
                   setwmm->actions.addts.tspec.tsinfo.PSB,
                   setwmm->actions.addts.tspec.tsinfo.UP,
                   setwmm->actions.addts.tspec.tsinfo.infoAck,
                   setwmm->actions.addts.tspec.tsinfo.bstSzDef,
                   setwmm->actions.addts.tspec.Fixed,
                   setwmm->actions.addts.tspec.size,
                   setwmm->actions.addts.tspec.maxsize,
                   setwmm->actions.addts.tspec.min_srvc,
                   setwmm->actions.addts.tspec.max_srvc,
                   setwmm->actions.addts.tspec.inactivity,
                   setwmm->actions.addts.tspec.suspension,
                   setwmm->actions.addts.tspec.srvc_strt_tim,
                   setwmm->actions.addts.tspec.mindatarate,
                   setwmm->actions.addts.tspec.meandatarate,
                   setwmm->actions.addts.tspec.peakdatarate,
                   setwmm->actions.addts.tspec.burstsize,
                   setwmm->actions.addts.tspec.delaybound,
                   setwmm->actions.addts.tspec.PHYrate,
                   setwmm->actions.addts.tspec.sba,
                   setwmm->actions.addts.tspec.medium_time,
                   setwmm->actions.addts.accesscat);

            wmmtspec_t *addts = &setwmm->actions.addts.tspec;

            //tspec should be set here.
            if (setwmm->actions.addts.psb_flag) {
                wSPRINTF(buf, "addts token %d,"
                    "tid %d,dir %d,psb %d,up %d,fixed %d,size %d,maxsize %d,maxsrvint %d,"
                    "minsrvint %d,inact %d,suspension %d,srvstarttime %d,minrate %d,meanrate %d,"
                    "peakrate %d,burst %d,delaybound %d,phyrate %d,sba %f,mediumtime %d",
                    setwmm->actions.addts.dialog_token, addts->tsinfo.TID, addts->tsinfo.direction,
                    addts->tsinfo.PSB, addts->tsinfo.UP, addts->Fixed, addts->size, addts->maxsize,
                    addts->max_srvc, addts->min_srvc, addts->inactivity, addts->suspension,
                    addts->srvc_strt_tim, addts->mindatarate, addts->meandatarate, addts->peakdatarate,
                    addts->burstsize, addts->delaybound, addts->PHYrate, addts->sba, addts->medium_time);
            } else {
                wSPRINTF(buf, "addts token %d,"
                    "tid %d,dir %d,up %d,fixed %d,size %d,maxsize %d,maxsrvint %d,"
                    "minsrvint %d,inact %d,suspension %d,srvstarttime %d,minrate %d,meanrate %d,"
                    "peakrate %d,burst %d,delaybound %d,phyrate %d,sba %f,mediumtime %d",
                    setwmm->actions.addts.dialog_token, addts->tsinfo.TID, addts->tsinfo.direction,
                    addts->tsinfo.UP, addts->Fixed, addts->size, addts->maxsize,
                    addts->max_srvc, addts->min_srvc, addts->inactivity, addts->suspension,
                    addts->srvc_strt_tim, addts->mindatarate, addts->meandatarate, addts->peakdatarate,
                    addts->burstsize, addts->delaybound, addts->PHYrate, addts->sba, addts->medium_time);
            }

            shell(IWPRIV" %s driver \"%s\"", setwmm->intf, buf);
        }
        else if (setwmm->action == WMMAC_DELTS)
        {
            // send del tspec
            wSPRINTF(buf, "delts tid %d", setwmm->actions.delts);

            shell(IWPRIV" %s driver \"%s\"", setwmm->intf, buf);
        }

        setwmmResp->status = STATUS_COMPLETE;
        break;

    case GROUP_WMMCONF:
        sprintf(gCmdStr, "iwconfig %s rts %d",
                ifname,setwmm->actions.config.rts_thr);

        sret = system(gCmdStr);
        sprintf(gCmdStr, "iwconfig %s frag %d",
                ifname,setwmm->actions.config.frag_thr);

        sret = system(gCmdStr);
        sprintf(gCmdStr, "iwpriv %s wmmcfg %d",
                ifname, setwmm->actions.config.wmm);

        sret = system(gCmdStr);
        setwmmResp->status = STATUS_COMPLETE;
        break;

    default:
        DPRINT_ERR(WFA_ERR, "The group %d is not supported\n",setwmm->group);
        setwmmResp->status = STATUS_ERROR;
        break;

    }

    wfaEncodeTLV(WFA_STA_SET_WMM_RESP_TLV, 4, (BYTE *)setwmmResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
#endif

    return WFA_SUCCESS;
}

int wfaStaSendNeigReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *sendNeigReqResp = &gGenericResp;

    /*
     *  run your device to send NEIGREQ
     */

    sendNeigReqResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SEND_NEIGREQ_RESP_TLV, 4, (BYTE *)sendNeigReqResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSetEapFAST(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    //caStaSetEapFAST_t *setFAST= (caStaSetEapFAST_t *)caCmdBuf;
    dutCmdResponse_t *setEapFastResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetEapFAST ...\n");

    setEapFastResp->status = STATUS_INVALID;
    wfaEncodeTLV(WFA_STA_SET_EAPFAST_RESP_TLV, 4, (BYTE *)setEapFastResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSetEapAKA(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEapAKA_t *setAKA = (caStaSetEapAKA_t *)caCmdBuf;
    char *intf = get_param_str("intf");
    dutCmdResponse_t *setEapAkaResp = &gGenericResp;
    int id;

    if (setAKA) {
        DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetEapAKA ...\n");
        set_param_str("ssid", setAKA->ssid);
        set_param_str("username", setAKA->username);
        set_param_str("password", setAKA->passwd);
        set_param_str("keyMgmtType", setAKA->keyMgmtType);
        set_param_str("encptype", setAKA->encrptype);
        set_param_val("pmf", setAKA->pmf);
        intf = setAKA->intf;
    }

    id = add_network_common(intf);
    if (id < 0) {
        setEapAkaResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_eap_common(intf, id) != STATUS_COMPLETE) {
        setEapAkaResp->status = STATUS_ERROR;
        goto done;
    }

    const char *eapMethod = get_param_str("username") &&
            get_param_str("username")[0] == '6' ? "AKA'" : "AKA";
    if (set_network(intf, id, "eap", eapMethod) < 0) {
        setEapAkaResp->status = STATUS_ERROR;
        goto done;
    }

    setEapAkaResp->status = STATUS_COMPLETE;
done:
    wfaEncodeTLV(WFA_STA_SET_EAPAKA_RESP_TLV, 4, (BYTE *)setEapAkaResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSetSystime(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetSystime_t *systime = (caStaSetSystime_t *)caCmdBuf;
    dutCmdResponse_t *setSystimeResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetSystime ... time=%d-%d-%d %d:%d:%d\n",
        systime->year, systime->month, systime->date,
        systime->hours, systime->minutes, systime->seconds);

    // set date by command: date MMDDhhmm[[CC]YY][.ss]
    sprintf(gCmdStr, "date %02d%02d%02d%02d%d.%02d",
        systime->month, systime->date, systime->hours,
        systime->minutes, systime->year, systime->seconds);
    sret = system(gCmdStr);

    wpa_command("wlan0", "PMKSA_FLUSH");

    setSystimeResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_SYSTIME_RESP_TLV, 4, (BYTE *)setSystimeResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

#ifdef WFA_STA_TB
int wfaStaPresetParams(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *PresetParamsResp = &gGenericResp;
    caStaPresetParameters_t *presetParams = (caStaPresetParameters_t *)caCmdBuf;
    char buf[1024];

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaPresetParameters function ... intf=%s,supp=%d,prog=%d\n",
        presetParams->intf, presetParams->supplicant, presetParams->program);

    wpa_command("wlan0", "PMKSA_FLUSH");

    // disable scan random mac for 11n
    wpa_command(presetParams->intf, "MAC_RAND_SCAN all enable=0");

    switch (presetParams->program) {
    case PROG_TYPE_HE:
    case PROG_TYPE_MBO:
    {
        if (presetParams->mboRoaming != 0) {
            wSNPRINTF(buf, sizeof(buf), "SET roaming %d",
                presetParams->mboRoaming == eEnable ? 1 : 0);

            if (wpa_command(presetParams->intf, buf) < 0) {
                DPRINT_INFO(WFA_ERR, "set roaming fail %d", presetParams->mboRoaming);
            }
        }

        if (presetParams->chans.chPrefNum)
            mbo_set_non_pref_chan(presetParams->intf, &presetParams->chans);

        if (presetParams->cellularDataCap)
            mbo_set_cellular_data_cap(presetParams->intf, presetParams->cellularDataCap);
    }
    break;

    default:
        break;
    }

    PresetParamsResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_PRESET_PARAMETERS_RESP_TLV, 4, (BYTE *)PresetParamsResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSet11n(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *v11nParamsResp = &gGenericResp;

    v11nParamsResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, 4, (BYTE *)v11nParamsResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
    return WFA_SUCCESS;
}
int wfaStaSetWireless(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetWireless_t *setWireless = (caStaSetWireless_t *)caCmdBuf;
    dutCmdResponse_t *staWirelessResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetWireless function ... program=%s\n", setWireless->program);

    if (setWireless->bcc_mode)
        set_param_val("bcc_mode", setWireless->bcc_mode);
    if (setWireless->mcs_fixedrate) {
        set_param_val("mcs_fixedrate", setWireless->mcs_fixedrate);
        set_param_val("mcs_fixedrate_rate", setWireless->mcs_fixedrate_rate);
    }
    if (str_same(setWireless->addba_req_size, "gt64") || str_same(setWireless->addba_req_size, "256")) {
        /* Set ADDBA buffer size to 256 */
        shell(IWPRIV" %s driver \"SET_BA_SIZE 256 \"", setWireless->intf);
    }
    if (str_same(setWireless->addba_resp_size, "gt64")) {
        /* Set BA resp buffer size to 256 */
        shell(IWPRIV" %s driver \"SET_BA_SIZE 256 \"", setWireless->intf);
    }

    if (setWireless->bw_sgnl) {
        shell(IWPRIV" %s driver \"set_cfg SigTaRts 1\"", setWireless->intf);
        set_param_val("bw_sgnl", setWireless->bw_sgnl);
    }
    if (setWireless->dyn_bw_sgnl) {
        shell(IWPRIV" %s driver \"set_cfg DynBwRts 1\"", setWireless->intf);
    }

    if (setWireless->ulTxBeforeTrig == WFA_ENABLED)
        setTxPPDUEnable(1);
    else if (setWireless->ulTxBeforeTrig == WFA_DISABLED)
        setTxPPDUEnable(0);

    if (setWireless->he_mcs_map >= 0 && setWireless->he_mcs_map <= 2)
        shell(IWPRIV" %s driver \"set_mcs_map %d\"", setWireless->intf, setWireless->he_mcs_map);

    if (setWireless->txsp_stream == 1 || setWireless->txsp_stream == 2)
#ifndef _FREERTOS
        shell(IWPRIV" %s driver \"set_nss %d\"", setWireless->intf, setWireless->txsp_stream);
#else
        shell(IWPRIV" %s driver \"set_nss 1\"", setWireless->intf);
#endif

    /* default capability of fullBW_ULMUMIMO is enabled*/
    if (setWireless->fullBW_ULMUMIMO == eEnable) {
        shell(IWPRIV" %s driver \"set_fullBW_ULMUMIMO 1\"", setWireless->intf);
    } else if (setWireless->fullBW_ULMUMIMO == eDisable) {
        shell(IWPRIV" %s driver \"set_fullBW_ULMUMIMO 0\"", setWireless->intf);
    }

    /*
    if (setWireless->TWTinfoFrameTx == eEnable) {
        shell(IWPRIV" %s driver \"set_TWT_Frame_TX 1\"", setWireless->intf);
    } else if (setWireless->TWTinfoFrameTx == eDisable) {
        shell(IWPRIV" %s driver \"set_TWT_Frame_TX 0\"", setWireless->intf);
    }
    */

    staWirelessResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_WIRELESS_RESP_TLV, 4, (BYTE *)staWirelessResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
    return WFA_SUCCESS;
}

int wfaStaSendADDBA(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *staSendADDBAResp = &gGenericResp;

    staSendADDBAResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_SEND_ADDBA_RESP_TLV, 4, (BYTE *)staSendADDBAResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
    return WFA_SUCCESS;
}

int wfaStaSetRIFS(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *staSetRIFSResp = &gGenericResp;

    staSetRIFSResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_RIFS_TEST_RESP_TLV, 4, (BYTE *)staSetRIFSResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSendCoExistMGMT(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *staSendMGMTResp = &gGenericResp;

    staSendMGMTResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SEND_COEXIST_MGMT_RESP_TLV, 4, (BYTE *)staSendMGMTResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;

}

int wfaStaResetDefault(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaResetDefault_t *reset = (caStaResetDefault_t *)caCmdBuf;
    dutCmdResponse_t *ResetResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaResetDefault ... prog=%s\n", reset->prog);

    remove_wpa_networks(reset->intf);

    reset_param();
    set_param_str("prog", reset->prog);
    gDut.program = str2prog(reset->prog);

    // disable scan random mac
    wpa_command(reset->intf, "MAC_RAND_SCAN all enable=0");

    if (gDut.program == PROG_TYPE_PMF)
        wpa_command(reset->intf, "STA_AUTOCONNECT 0");

    if (gDut.program == PROG_TYPE_MBO || gDut.program == PROG_TYPE_HE) {
        wpa_command(reset->intf, "SET gas_address3 0");
        wpa_command(reset->intf, "SET roaming 1");
        wpa_command(reset->intf, "SET interworking 1");
        // workaround for wireshark which can't recognize empty non_pref_chan
        wpa_command(reset->intf, "SET non_pref_chan 81:5:10:2");
    }

    if(strncmp(reset->prog, "VHT", 3) == 0) {
        shell(IWPRIV" %s driver \"set_cfg SigTaRts 0\"", reset->intf);
        shell(IWPRIV" %s driver \"set_chip edcca 0 0xFF\"", reset->intf);
        shell(IWPRIV" %s driver \"set_cfg DynBwRts 0\"", reset->intf);
    }

#ifdef CONFIG_MTK_WFD

    if (gDut.program == PROG_TYPE_WFD) {
        if(wfaMtkWfdCmd_rtspTeardown() != 0) {
            DPRINT_ERR(WFA_ERR, "Fail to send RTSP Teardown\n");
        }
        DPRINT_ERR(WFA_ERR, "Send RTSP Teardown [DONE] \n");

        wpa_command(reset->intf, "P2P_STOP_FIND");
        wpa_command(reset->intf, "P2P_FLUSH");

        /* set wfd_devType=1, wfd_sessionAvail=1, wfd_maxThroughput=300, wfd_rtspPort=554 */
        wpa_command(reset->intf, "WFD_SUBELEM_SET 0 00060011022a012c");
        wpa_command(reset->intf, "set device_name MTK-DTV-Sink");

        /* enable driver to append WFD IE to the association response(TC 6.1.2) */
        shell(IWPRIV" %s driver \"miracast 2\"", reset->intf);

        DPRINT_INFO(WFA_OUT, "==== Enable WFD function ====\n");
        /* enable WiFi Display */
        wpa_command(reset->intf, "SET wifi_display 1");

        /* disconnect from the AP and clean profile (TC 6.1.17A/B) */
        wpa_command(get_sta_intf(reset->intf), "DISCONNECT");
        wpa_command(get_sta_intf(reset->intf), "REMOVE_NETWORK all");

    }
    gDut.p2p_params.group_started = 0;
    gDut.p2p_params.role = -1;

#endif

    if (gDut.program == PROG_TYPE_HE) {
        setTxPPDUEnable(1);
        shell(IWPRIV " %s driver \"set_mcs_map 3\"", reset->intf);
#ifndef _FREERTOS
        shell(IWPRIV " %s driver \"set_nss 2\"", reset->intf);
#else
        shell(IWPRIV" %s driver \"set_nss 1\"", reset->intf);
#endif
        wpa_command(reset->intf, "SET sae_pwe 2");
        wpa_command(reset->intf, "BSS_FLUSH");
        shell(IWPRIV" %s driver \"set_chip send_omi 0 1 0x411 0\"", reset->intf);
        shell(IWPRIV" %s driver \"set_fullBW_ULMUMIMO 1\"", reset->intf);
    }

    shell(IWPRIV" %s driver \"set_mcr 0x820E2028 0x00000000\"", reset->intf);
    shell(MTKINBANDCMD" %s %s", reset->intf, MTK_INBAND_INIT);

    if (shell(DHCPRESET) != WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "reset sta dhcp fail\n");

    ResetResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_RESET_DEFAULT_RESP_TLV, 4, (BYTE *)ResetResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

#else

int wfaStaTestBedCmd(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *staCmdResp = &gGenericResp;

    wfaEncodeTLV(WFA_STA_DISCONNECT_RESP_TLV, 4, (BYTE *)staCmdResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}
#endif

/*
 * This is used to send a frame or action frame
 */
int wfaStaDevSendFrame(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *cmd = (dutCommand_t *)caCmdBuf;
    /* uncomment it if needed */
    char *ifname = cmd->intf;
    dutCmdResponse_t *devSendResp = &gGenericResp;
    caStaDevSendFrame_t *sf = &cmd->cmdsu.sf;
    char buf[256];

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaDevSendFrame function ...prog=%d\n", sf->program);
    /* processing the frame */

    switch(sf->program)
    {
    case PROG_TYPE_PMF:
    {
        pmfFrame_t *pmf = &sf->frameType.pmf;
        switch(pmf->eFrameName)
        {
        case PMF_TYPE_DISASSOC:
        {
            /* use the protected to set what type of key to send */

        }
        break;
        case PMF_TYPE_DEAUTH:
        {

        }
        break;
        case PMF_TYPE_SAQUERY:
        {

        }
        break;
        case PMF_TYPE_AUTH:
        {
        }
        break;
        case PMF_TYPE_ASSOCREQ:
        {
        }
        break;
        case PMF_TYPE_REASSOCREQ:
        {
        }
        break;
        }
    }
    break;
    case PROG_TYPE_TDLS:
    {
        tdlsFrame_t *tdls = &sf->frameType.tdls;
        DPRINT_INFO(WFA_OUT, "tdls frame=%d\n", tdls->eFrameName);
        /* use the peer mac address to send the frame */
        switch(tdls->eFrameName)
        {
            case TDLS_TYPE_DISCOVERY:
                wSPRINTF(buf, "TDLS_DISCOVER %s", tdls->peer);
                break;
            case TDLS_TYPE_SETUP:
                wSPRINTF(buf, "TDLS_SETUP %s", tdls->peer);
                break;
            case TDLS_TYPE_TEARDOWN:
                wSPRINTF(buf, "TDLS_TEARDOWN %s", tdls->peer);
                break;
            case TDLS_TYPE_CHANNELSWITCH:
                break;
            case TDLS_TYPE_NULLFRAME:
                break;
        }
        wpa_command(cmd->intf, buf);
    }
    break;
    case PROG_TYPE_VENT:
    {
        ventFrame_t *vent = &sf->frameType.vent;

        DPRINT_INFO(WFA_OUT, "Vent type %d, ssid %s\n", vent->type, vent->ssid);
        switch(vent->type)
        {
        case VENT_TYPE_NEIGREQ:
            wSPRINTF(buf, "DRIVER NEIGHBOR-REQUEST SSID=%s", vent->ssid);
            break;
        case VENT_TYPE_TRANSMGMT:
            wSPRINTF(buf, "DRIVER BSS-TRANSITION-QUERY reason=6");
            break;
        }
        wpa_command(cmd->intf, buf);
    }
    break;
    case PROG_TYPE_WFD:
    {
        wfdFrame_t *wfd = &sf->frameType.wfd;
        switch(wfd->eframe)
        {
        case WFD_FRAME_PRBREQ:
        {
            /* send probe req */
        }
        break;

        case WFD_FRAME_PRBREQ_TDLS_REQ:
        {
            /* send tunneled tdls probe req  */
        }
        break;

        case WFD_FRAME_11V_TIMING_MSR_REQ:
        {
            /* send 11v timing mearurement request */
        }
        break;

        case WFD_FRAME_RTSP:
        {
            /* send WFD RTSP messages*/
            // fetch the type of RTSP message and send it.
            switch(wfd->eRtspMsgType)
            {
            case WFD_RTSP_PAUSE:
#ifdef CONFIG_MTK_WFD
                //send RTSP PAUSE
                {
                    if (wfaMtkWfdCmd_rtspPause() != 0)
                        DPRINT_ERR(WFA_ERR, "Fail to send RTSP PAUSE\n");
                }
#endif
                break;
            case WFD_RTSP_PLAY:
                //send RTSP PLAY
#ifdef CONFIG_MTK_WFD
                {
                    if (wfaMtkWfdCmd_rtspPlay() != 0)
                        DPRINT_ERR(WFA_ERR, "Fail to send RTSP PLAY\n");
                }
#endif
                break;
            case WFD_RTSP_TEARDOWN:
                //send RTSP TEARDOWN
#ifdef CONFIG_MTK_WFD
                {
                    if (wfaMtkWfdCmd_rtspTeardown() != 0)
                        DPRINT_ERR(WFA_ERR, "Fail to send RTSP Teardown\n");
                }
#endif
                break;
            case WFD_RTSP_TRIG_PAUSE:
                //send RTSP TRIGGER PAUSE
                break;
            case WFD_RTSP_TRIG_PLAY:
                //send RTSP TRIGGER PLAY
                break;
            case WFD_RTSP_TRIG_TEARDOWN:
                //send RTSP TRIGGER TEARDOWN
                break;
            case WFD_RTSP_SET_PARAMETER:
                //send RTSP SET PARAMETER
                if (wfd->eSetParams == WFD_CAP_UIBC_KEYBOARD)
                {
                    //send RTSP SET PARAMETER message for UIBC keyboard
#ifdef CONFIG_MTK_WFD
                    DPRINT_INFO(WFA_OUT, "%s: UibcCapUpdate = Keyboard\n", __FUNCTION__);
                    if (wfaMtkWfdCmd_rtspUibcCapUpdate(WFD_CAP_UIBC_KEYBOARD) != 0)
                        DPRINT_ERR(WFA_ERR, "Fail to update RTSP UIBC Cap to Keyboard\n");
#endif
                }
                if (wfd->eSetParams == WFD_CAP_UIBC_MOUSE)
                {
                    //send RTSP SET PARAMETER message for UIBC Mouse
#ifdef CONFIG_MTK_WFD
                    DPRINT_INFO(WFA_OUT, "%s: UibcCapUpdate = Mouse\n", __FUNCTION__);
                    if (wfaMtkWfdCmd_rtspUibcCapUpdate(WFD_CAP_UIBC_MOUSE) != 0)
                        DPRINT_ERR(WFA_ERR, "Fail to update RTSP UIBC Cap to Mouse\n");
#endif
                }
                else if (wfd->eSetParams == WFD_CAP_RE_NEGO)
                {
                    //send RTSP SET PARAMETER message Capability re-negotiation
                }
                else if (wfd->eSetParams == WFD_STANDBY)
                {
                    //send RTSP SET PARAMETER message for standby
#ifdef CONFIG_MTK_WFD
                    if (wfaMtkWfdCmd_rtspEnterStandby() != 0)
                    {
                        DPRINT_ERR(WFA_ERR, "%s: enterStandy mode failed!\n", __FUNCTION__);
                    }
                    else
                        DPRINT_INFO(WFA_OUT, "%s: enterStandy mode success!\n", __FUNCTION__);
#endif
                }
                else if (wfd->eSetParams == WFD_UIBC_SETTINGS_ENABLE)
                {
                    //send RTSP SET PARAMETER message for UIBC settings enable
                }
                else if (wfd->eSetParams == WFD_UIBC_SETTINGS_DISABLE)
                {
                    //send RTSP SET PARAMETER message for UIBC settings disable
                }
                else if (wfd->eSetParams == WFD_ROUTE_AUDIO)
                {
                    //send RTSP SET PARAMETER message for route audio
                }
                else if (wfd->eSetParams == WFD_3D_VIDEOPARAM)
                {
                    //send RTSP SET PARAMETER message for 3D video parameters
                }
                else if (wfd->eSetParams == WFD_2D_VIDEOPARAM)
                {
                    //send RTSP SET PARAMETER message for 2D video parameters
                }
                break;
            }
        }
        break;
        }
    }
    break;
    /* not need to support HS2 release 1, due to very short time period  */
    case PROG_TYPE_HS2_R2:
    {
        /* type of frames */
        hs2Frame_t *hs2 = &sf->frameType.hs2_r2;
        switch(hs2->eframe)
        {
        case HS2_FRAME_ANQPQuery:
        {

        }
        break;
        case HS2_FRAME_DLSRequest:
        {

        }
        break;
        case HS2_FRAME_GARPReq:
        {

        }
        break;
        case HS2_FRAME_GARPRes:
        {
        }
        break;
        case HS2_FRAME_NeighAdv:
        {
        }
        case HS2_FRAME_ARPProbe:
        {
        }
        case HS2_FRAME_ARPAnnounce:
        {

        }
        break;
        case HS2_FRAME_NeighSolicitReq:
        {

        }
        break;
        case HS2_FRAME_ARPReply:
        {

        }
        break;
        }

    }/*  PROG_TYPE_HS2-R2  */
    break;
    case PROG_TYPE_MBO:
    {
        mboFrame_t *mbo = &sf->frameType.mbo;
        char buf[1024];

        switch(mbo->frame)
        {
            case MBO_FRAME_BTM_QUERY:
            {
                wSNPRINTF(buf, sizeof(buf), "WNM_BSS_QUERY %d", mbo->btmQueryReasonCode);
                if (wpa_command(cmd->intf, buf) < 0) {
                    DPRINT_INFO(WFA_ERR, "BTM query failed\n");
                } else {
                    DPRINT_INFO(WFA_OUT, "BTM query sent\n");
                }
            }
            break;
            case MBO_FRAME_ANQP_QUERY:
            {
                int valid = WFA_SUCCESS;

                switch (mbo->anqpQueryId) {
                case MBO_ANQP_QUERY_ID_NEIGHBOR_REPOERT_REQ:
                {
                    wSNPRINTF(buf, sizeof(buf), "ANQP_GET %s 272", mbo->dest);
                }
                break;
                case MBO_ANQP_QUERY_ID_QUERY_LIST_WITH_CELL_PREF:
                {
                    wSNPRINTF(buf, sizeof(buf), "ANQP_GET %s 272,mbo:2", mbo->dest);
                }
                break;
                default:
                    DPRINT_INFO(WFA_ERR, "Invalid ANQPQuery_ID %d", mbo->anqpQueryId);
                    valid = WFA_FAILURE;
                    break;
                }

                if (valid == WFA_SUCCESS) {
                    /* Set gas_address3 field to IEEE 802.11-2012 standard compliant form
                     * (Address3 = Wildcard BSSID when sent to not-associated AP;
                     * if associated, AP BSSID).
                     */
                    if (wpa_command(cmd->intf, "SET gas_address3 1") < 0)
                        DPRINT_INFO(WFA_ERR, "Failed to set gas_address3\n");

                    if (wpa_command(cmd->intf, buf) < 0)
                        DPRINT_INFO(WFA_ERR, "ANQP query failed\n");
                }
            }
            break;
        }
    }
    break;
    case PROG_TYPE_HE: {
        heFrame_t *he = &sf->frameType.he;

        DPRINT_INFO(WFA_ERR, "send HE frame %d \n", he->eframe);
        switch (he->eframe) {
        case HE_TYPE_ACTION:
            break;
        case HE_TYPE_TRIGGER:
            break;
        case HE_TYPE_DISASSOC:
            if(he->eProtected == PMF_PROT_CORRECTKEY)
                shell(IWPRIV " %s driver \"set_sw_ctrl 0x20000004 0\"", ifname);
            else if(he->eProtected == PMF_PROT_INCORRECTKEY)
                shell(IWPRIV " %s driver \"set_sw_ctrl 0x20000005 0\"\n", ifname);
            else if(he->eProtected == PMF_PROT_UNPROTECTED)
                shell(IWPRIV " %s driver \"set_sw_ctrl 0x20000006 0\"\n", ifname);
        break;
        case HE_TYPE_DEAUTH:
                break;
        }
    }
    break;
    case PROG_TYPE_GEN:
    {
        /* General frames */
    }
    default:
        break;
    }
    devSendResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_DEV_SEND_FRAME_RESP_TLV, 4, (BYTE *)devSendResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * This is used to set a temporary MAC address of an interface
 */
int wfaStaSetMacAddr(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    // Uncomment it if needed
    //dutCommand_t *cmd = (dutCommand_t *)caCmdBuf;
    // char *ifname = cmd->intf;
    dutCmdResponse_t *staCmdResp = &gGenericResp;
    // Uncomment it if needed
    //char *macaddr = &cmd->cmdsu.macaddr[0];

    wfaEncodeTLV(WFA_STA_SET_MAC_ADDRESS_RESP_TLV, 4, (BYTE *)staCmdResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}


int wfaStaDisconnect(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *disc = (dutCommand_t *)caCmdBuf;
    char *intf = disc->intf;
    dutCmdResponse_t *staDiscResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaDisconnect ...\n");

    wpa_command(intf, "DISCONNECT");

    /*
     * remove this due to UCC, e.g. 11n 5.2.53, won't guantee that it will
     * set network parameters after issue disconnect
     *
     * remove_wpa_networks(intf);
     */
    shell(MTKINBANDCMD" %s %s", intf, MTK_INBAND_INIT);

    staDiscResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_DISCONNECT_RESP_TLV, 4, (BYTE *)staDiscResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/* Execute CLI, read the status from Environment variable */
int wfaExecuteCLI(char *CLI)
{
    char *retstr;

    sret = system(CLI);

    retstr = getenv("WFA_CLI_STATUS");
    printf("cli status %s\n", retstr);
    return atoi(retstr);
}

void wfaSendPing(tgPingStart_t *staPing, float *interval, int streamid)
{
    int totalpkts, tos=-1;
    char cmdStr[256];
    char bflag[] = "  ";

    totalpkts = (int)(staPing->duration * staPing->frameRate);
    Totalpkts = totalpkts;

    if (staPing->dscp >= 0) {
        tos= convertDscpToTos(staPing->dscp);
        if (tos < 0)
            DPRINT_WARNING(WFA_WNG, "invalid tos converted, dscp=%d\n",
                    staPing->dscp);
    }
    DPRINT_INFO(WFA_OUT, "Entering %s(), streamid:%d iptype:%d tos:%d\n",
            __func__, streamid, staPing->iptype, tos);
#if defined(_FREERTOS)
	/* FREERTOS ping command: ping <addr> <count> <pkt_len>*/
    DPRINT_INFO(WFA_OUT, "%s: interval: %f", __func__, *interval);
    g_ping_interval = *interval * 1000;
    if (staPing->iptype == 2)
    {
#if 0
        if (tos > 0)
            sprintf(cmdStr, "ping %s %s %i %i",
                    bflag, staPing->dipaddr, totalpkts, staPing->frameSize);
        else
            sprintf(cmdStr, "ping %s %s %i %i",
                    bflag, staPing->dipaddr, totalpkts, staPing->frameSize);
        sret = system(cmdStr);
#else
        if (tos > 0)
            ping_request(totalpkts, staPing->dipaddr, PING_IP_ADDR_V4, staPing->frameSize, NULL);
        else
            ping_request(totalpkts, staPing->dipaddr, PING_IP_ADDR_V4, staPing->frameSize, NULL);
#endif
        DPRINT_INFO(WFA_OUT, "\nCS : The command string is %s\n",cmdStr);
    }
    else
    {
#if 0
        if (tos > 0)
            sprintf(cmdStr, "ping %s %s %i %i",
                    bflag, staPing->dipaddr, totalpkts, staPing->frameSize);
        else
            sprintf(cmdStr, "ping %s %s %i %i",
                    bflag, staPing->dipaddr, totalpkts, staPing->frameSize);
        sret = system(cmdStr);
#else
        if (tos > 0)
            ping_request(totalpkts, staPing->dipaddr, PING_IP_ADDR_V4, staPing->frameSize, NULL);
        else
            ping_request(totalpkts, staPing->dipaddr, PING_IP_ADDR_V4, staPing->frameSize, NULL);
#endif
	DPRINT_INFO(WFA_OUT, "\nCS : The command string is %s\n",cmdStr);
    }

#else
    if (staPing->iptype == 2) {
        if (tos > 0)
            wSNPRINTF(cmdStr, sizeof(cmdStr), "echo streamid=%i > " BIN_DIR "/spout_%d.txt;" WFAPING6 " %s -i %f -c %i -Q %d -s %i -q %s >> " BIN_DIR "/spout_%d.txt 2>/dev/null",
                    streamid, streamid, bflag, *interval, totalpkts, tos,  staPing->frameSize, staPing->dipaddr, streamid);
        else
            wSNPRINTF(cmdStr, sizeof(cmdStr), "echo streamid=%i > " BIN_DIR "/spout_%d.txt;" WFAPING6 " %s -i %f -c %i -s %i -q %s >> " BIN_DIR "/spout_%d.txt 2>/dev/null",
                    streamid, streamid, bflag, *interval, totalpkts, staPing->frameSize, staPing->dipaddr, streamid);
    } else {
        if (tos > 0)
            wSNPRINTF(cmdStr, sizeof(cmdStr), "echo streamid=%i > " BIN_DIR "/spout_%d.txt;" WFAPING " %s -i %f -c %i -Q %d -s %i -q %s >> " BIN_DIR "/spout_%d.txt 2>/dev/null",
                    streamid, streamid, bflag, *interval, totalpkts, tos, staPing->frameSize, staPing->dipaddr,streamid);
        else
            wSNPRINTF(cmdStr, sizeof(cmdStr), "echo streamid=%i > " BIN_DIR "/spout_%d.txt;" WFAPING " %s -i %f -c %i -s %i -q %s >> " BIN_DIR "/spout_%d.txt 2>/dev/null",
                    streamid, streamid, bflag, *interval, totalpkts, staPing->frameSize, staPing->dipaddr, streamid);
    }
    DPRINT_INFO(WFA_OUT, "cmdStr: %s\n", cmdStr);
    system(cmdStr);

    sprintf(cmdStr, UPDATEPID " " BIN_DIR "/spout_%d.txt", streamid);
    DPRINT_INFO(WFA_OUT, "cmdStr: %s\n", cmdStr);
    system(cmdStr);
#endif
}

int wfaStopPing(dutCmdResponse_t *stpResp, int streamid)
{
#if defined(_FREERTOS)
    stpResp->cmdru.pingStp.sendCnt = Totalpkts;
    stpResp->cmdru.pingStp.repliedCnt = g_ping_recv;
    /* Kill the ping task */
    g_task_handle = NULL;
    g_ping_recv = 0;
#else
    char strout[256];
    FILE *tmpfile = NULL;
    char cmdStr[256];

    DPRINT_INFO(WFA_OUT, "Entering %s(), streamid:%d\n",
            __func__, streamid);

    sprintf(cmdStr, GETPID " " BIN_DIR "/spout_%d.txt " BIN_DIR "/pid.txt",
            streamid);
    DPRINT_INFO(WFA_OUT, "cmdStr: %s\n", cmdStr);
    system(cmdStr);

    sprintf(cmdStr, STOPPING " " BIN_DIR "/pid.txt ; sleep 2");
    DPRINT_INFO(WFA_OUT, "cmdStr: %s\n", cmdStr);
    system(cmdStr);

    sprintf(cmdStr, GETPSTATS " " BIN_DIR "/spout_%d.txt " BIN_DIR "/stpsta.txt", streamid);
    DPRINT_INFO(WFA_OUT, "cmdStr: %s\n", cmdStr);
    system(cmdStr);

    tmpfile = fopen(BIN_DIR "/stpsta.txt", "r+");
    if (tmpfile == NULL)
        return WFA_FAILURE;
    if (fscanf(tmpfile, "%255s", strout) != EOF) {
        if (*strout == '\0')
            stpResp->cmdru.pingStp.sendCnt = 0;
        else
            stpResp->cmdru.pingStp.sendCnt = atoi(strout);
    }
    DPRINT_INFO(WFA_OUT, "sent count:%d\n", stpResp->cmdru.pingStp.sendCnt);
    if (fscanf(tmpfile, "%255s", strout) != EOF) {
        if (*strout == '\0')
            stpResp->cmdru.pingStp.repliedCnt = 0;
        else
            stpResp->cmdru.pingStp.repliedCnt = atoi(strout);
    }
    DPRINT_INFO(WFA_OUT, "replied count:%d\n",
            stpResp->cmdru.pingStp.repliedCnt);
    fclose(tmpfile);

#endif
    return WFA_SUCCESS;
}

#ifdef CONFIG_MTK_P2P
/*
 * wfaStaGetP2pDevAddress():
 */
int wfaStaGetP2pDevAddress(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    dutCommand_t *getInfo = (dutCommand_t *)caCmdBuf;
    char buf[100];
    enum _response_staus status = STATUS_COMPLETE;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (get_wpa_status(getInfo->intf, "p2p_device_address", buf,
            sizeof(buf)) < 0) {
        status = STATUS_ERROR;
    } else {
        status = STATUS_COMPLETE;
        wSNPRINTF(infoResp->cmdru.devid, sizeof(infoResp->cmdru.devid),
                "%s", buf);
    }

    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_P2P_GET_DEV_ADDRESS_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaSetP2p():
 */
int wfaStaSetP2p(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaSetP2p_t *getStaSetP2p = (caStaSetP2p_t *)caCmdBuf;
    char buf[256];
    enum _response_staus status = STATUS_COMPLETE;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (getStaSetP2p->listen_chn_flag) {
        wSNPRINTF(buf, sizeof(buf), "P2P_SET listen_channel %d",
                getStaSetP2p->listen_chn);
        if (wpa_command(getStaSetP2p->intf, buf) != 0) {
            status = STATUS_ERROR;
            goto exit;
        }
    }

    if (getStaSetP2p->noa_duration_flag && getStaSetP2p->noa_count_flag &&
            getStaSetP2p->noa_interval_flag) {
        wSNPRINTF(buf, sizeof(buf), "P2P_SET noa %d,%d,%d",
                getStaSetP2p->noa_count,
                getStaSetP2p->noa_interval,
                getStaSetP2p->noa_duration);
        if (wpa_command(getStaSetP2p->intf, buf) != 0) {
            status = STATUS_ERROR;
            goto exit;
        }
    }

    if (getStaSetP2p->discoverability_flag) {
        wSNPRINTF(buf, sizeof(buf), "P2P_SET discoverability %d",
                getStaSetP2p->discoverability);
        if (wpa_command(getStaSetP2p->intf, buf) != 0) {
            status = STATUS_ERROR;
            goto exit;
        }
    }

    if (getStaSetP2p->presistent_flag) {
        gDut.p2p_params.persistent = getStaSetP2p->presistent;
    }

    if (getStaSetP2p->p2pmanaged_flag) {
        wSNPRINTF(buf, sizeof(buf), "P2P_SET managed %d",
                getStaSetP2p->p2pmanaged);
        if (wpa_command(getStaSetP2p->intf, buf) != 0) {
            status = STATUS_ERROR;
            goto exit;
        }
    }

    if (getStaSetP2p->go_apsd_flag) {
        wSNPRINTF(buf, sizeof(buf), "P2P_SET go_apsd %d",
                getStaSetP2p->go_apsd);
        if (wpa_command(getStaSetP2p->intf, buf) != 0) {
            status = STATUS_ERROR;
            goto exit;
        }
    }

    if (getStaSetP2p->crossconnection_flag) {
        wSNPRINTF(buf, sizeof(buf), "P2P_SET cross_connect %d",
                getStaSetP2p->crossconnection);
        if (wpa_command(getStaSetP2p->intf, buf) != 0) {
            status = STATUS_ERROR;
            goto exit;
        }
    }

    if (getStaSetP2p->p2p_mode_flag) {
        if (strcasecmp(getStaSetP2p->p2p_mode, "listen") == 0) {
            if (wpa_command(getStaSetP2p->intf, "P2P_SET disabled 0") != 0) {
                status = STATUS_ERROR;
                goto exit;
            }
            if (wpa_command(getStaSetP2p->intf, "P2P_LISTEN") != 0) {
                status = STATUS_ERROR;
                goto exit;
            }
            gDut.p2p_params.p2p_mode = P2P_MODE_LISTEN;
        } else if (strcasecmp(getStaSetP2p->p2p_mode, "Discover") == 0) {
            if (wpa_command(getStaSetP2p->intf, "P2P_SET disabled 0") != 0) {
                status = STATUS_ERROR;
                goto exit;
            }
            if (wpa_command(getStaSetP2p->intf, "P2P_FIND") != 0) {
                status = STATUS_ERROR;
                goto exit;
            }
            gDut.p2p_params.p2p_mode = P2P_MODE_DISCOVER;
        } else if (strcasecmp(getStaSetP2p->p2p_mode, "Idle") == 0) {
            if (wpa_command(getStaSetP2p->intf, "P2P_SET disabled 0") != 0) {
                status = STATUS_ERROR;
                goto exit;
            }
            if (wpa_command(getStaSetP2p->intf, "P2P_STOP_FIND") != 0) {
                status = STATUS_ERROR;
                goto exit;
            }
            gDut.p2p_params.p2p_mode = P2P_MODE_IDLE;
        } else if (strcasecmp(getStaSetP2p->p2p_mode, "Disable") == 0) {
            if (wpa_command(getStaSetP2p->intf, "P2P_SET disabled 1") != 0) {
                status = STATUS_ERROR;
                goto exit;
            }
            gDut.p2p_params.p2p_mode = P2P_MODE_DISABLE;
        } else {
            status = STATUS_ERROR;
            goto exit;
        }
    }

    if (getStaSetP2p->ext_listen_time_int_flag &&
            getStaSetP2p->ext_listen_time_period_flag) {
        wSNPRINTF(buf, sizeof(buf), "P2P_EXT_LISTEN %d %d",
                getStaSetP2p->ext_listen_time_period,
                getStaSetP2p->ext_listen_time_int);
        if (wpa_command(getStaSetP2p->intf, buf) != 0) {
            status = STATUS_ERROR;
            goto exit;
        }
    }

exit:
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_P2P_SETP2P_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return status;
}

/*
 * wfaStaP2pConnect():
 */
int wfaStaP2pConnect(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaP2pConnect_t *getStaP2pConnect = (caStaP2pConnect_t *)caCmdBuf;
    enum _response_staus status = STATUS_COMPLETE;
    char buf[256];
    struct wpa_ctrl *ctrl;
    int res;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (p2p_find_peer(getStaP2pConnect->intf, getStaP2pConnect->devId, 1) <= 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    switch (gDut.p2p_params.wps_method) {
    case WPS_METHOD_PBC:
        wSNPRINTF(buf, sizeof(buf), "P2P_CONNECT %s pbc join",
                getStaP2pConnect->devId);
        break;
    case WPS_METHOD_PIN_DISPLAY:
        wSNPRINTF(buf, sizeof(buf), "P2P_CONNECT %s %s display join",
                getStaP2pConnect->devId,
                gDut.p2p_params.wpsPin);
        break;
    case WPS_METHOD_KEYPAD:
        wSNPRINTF(buf, sizeof(buf), "P2P_CONNECT %s %s keypad join",
                getStaP2pConnect->devId,
                gDut.p2p_params.wpsPin);
        break;
    default:
        status = STATUS_ERROR;
        goto exit;
    }

    ctrl = open_wpa_mon(getStaP2pConnect->intf);
    if (ctrl == NULL) {
        DPRINT_ERR(WFA_ERR, "open wpa mon fail.\n");
        status = STATUS_ERROR;
        goto exit;
    }
    if (wpa_command(getStaP2pConnect->intf, buf) != 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    res = get_wpa_cli_event(ctrl, "P2P-GROUP-STARTED", buf, sizeof(buf));

    wpa_ctrl_detach(ctrl);
    wpa_ctrl_close(ctrl);

    if (res < 0) {
        DPRINT_ERR(WFA_ERR, "p2p connect fail.\n");
        status = STATUS_ERROR;
        goto exit;
    }

exit:
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_P2P_CONNECT_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return status;
}

/*
 * wfaStaStartAutoGo():
 */
int wfaStaStartAutoGo(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaStartAutoGo_t *getStaStartAutoGo = (caStaStartAutoGo_t *)caCmdBuf;
    int freq;
    enum _response_staus status = STATUS_COMPLETE;
    char buf[256];
    struct wpa_ctrl *ctrl;
    int res;
    char *pos, *ssid, *go_dev_addr;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (getStaStartAutoGo->oper_chn >= 1 && getStaStartAutoGo->oper_chn <= 13) {
        freq = 2412 + (getStaStartAutoGo->oper_chn - 1) * 5;
    } else if (getStaStartAutoGo->oper_chn == 14) {
        freq = 2484;
    } else if (getStaStartAutoGo->oper_chn >= 36 &&
            getStaStartAutoGo->oper_chn <= 165) {
        freq = 5000 + getStaStartAutoGo->oper_chn * 5;
    } else {
        status = STATUS_ERROR;
        goto exit;
    }

    if (getStaStartAutoGo->ssid_flag) {
        wSNPRINTF(buf, sizeof(buf), "P2P_SET ssid_postfix %s",
                getStaStartAutoGo->ssid);
    } else {
        wSNPRINTF(buf, sizeof(buf), "P2P_SET ssid_postfix ");
    }
    if (wpa_command(getStaStartAutoGo->intf, buf) != 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    if (wpa_command(getStaStartAutoGo->intf, "P2P_STOP_FIND") != 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    ctrl = open_wpa_mon(getStaStartAutoGo->intf);
    if (ctrl == NULL) {
        DPRINT_ERR(WFA_ERR, "open wpa mon fail.\n");
        status = STATUS_ERROR;
        goto exit;
    }

    wSNPRINTF(buf, sizeof(buf), "P2P_GROUP_ADD %sfreq=%d",
            gDut.p2p_params.persistent ? "persistent " : "",
            freq);
    if (wpa_command(getStaStartAutoGo->intf, buf) != 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    res = get_wpa_cli_event(ctrl, "P2P-GROUP-STARTED", buf, sizeof(buf));

    wpa_ctrl_detach(ctrl);
    wpa_ctrl_close(ctrl);

    if (res < 0) {
        DPRINT_ERR(WFA_ERR, "start auto GO fail.\n");
        status = STATUS_ERROR;
        goto exit;
    }

    DPRINT_INFO(WFA_OUT, "buf: %s\n", buf);

    ssid = strstr(buf, "ssid=\"");
    if (ssid == NULL) {
        DPRINT_ERR(WFA_ERR, "get ssid NULL.\n");
        status = STATUS_ERROR;
        goto exit;
    }
    ssid += 6;
    pos = strchr(ssid, '"');
    if (pos == NULL) {
        DPRINT_ERR(WFA_ERR, "get ssid terminator fail.\n");
        status = STATUS_ERROR;
        goto exit;
    }
    *pos++ = '\0';
    DPRINT_INFO(WFA_OUT, "ssid: %s\n", ssid);

    go_dev_addr = strstr(pos, "go_dev_addr=");
    if (go_dev_addr == NULL) {
        DPRINT_ERR(WFA_ERR, "get go_dev_addr NULL.\n");
        status = STATUS_ERROR;
        goto exit;
    }
    go_dev_addr += 12;
    go_dev_addr[WFA_P2P_DEVID_LEN - 1] = '\0';
    DPRINT_INFO(WFA_OUT, "go_dev_addr: %s\n", go_dev_addr);

    wSNPRINTF(infoResp->cmdru.grpid, sizeof(infoResp->cmdru.grpid), "%s %s",
            go_dev_addr, ssid);

#ifdef CONFIG_MTK_WFD
    if (sigma_mode == SIGMA_MODE_WFD)
    {
        static pthread_t wfdThread;

        DPRINT_INFO(WFA_OUT, "Create a thread for WFD connection!!\n");
        pthread_create(&wfdThread, NULL, &wfaStaWaitingWfdConnection_AutoGO, &getStaStartAutoGo->intf);
    }
#endif

exit:
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_P2P_START_AUTO_GO_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return status;
}

/*
 * wfaStaP2pStartGrpFormation():
 */
int wfaStaP2pStartGrpFormation(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    enum _response_staus status = STATUS_COMPLETE;
    caStaP2pStartGrpForm_t *getStaP2pStartGrpForm = (caStaP2pStartGrpForm_t *)caCmdBuf;
    int freq = 0;
    char buf[256];
    struct wpa_ctrl *ctrl = NULL;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (getStaP2pStartGrpForm->ssid_flag)
        wSNPRINTF(buf, sizeof(buf), "P2P_SET ssid_postfix %s",
                getStaP2pStartGrpForm->ssid);
    else
        wSNPRINTF(buf, sizeof(buf), "P2P_SET ssid_postfix ");
    if (wpa_command(getStaP2pStartGrpForm->intf, buf) < 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    if (getStaP2pStartGrpForm->oper_chn_flag) {
        if (getStaP2pStartGrpForm->oper_chn >= 1 && getStaP2pStartGrpForm->oper_chn <= 13) {
            freq = 2412 + (getStaP2pStartGrpForm->oper_chn - 1) * 5;
        } else if (getStaP2pStartGrpForm->oper_chn == 14) {
            freq = 2484;
        } else if (getStaP2pStartGrpForm->oper_chn >= 36 &&
                getStaP2pStartGrpForm->oper_chn <= 165) {
            freq = 5000 + getStaP2pStartGrpForm->oper_chn * 5;
        } else {
            status = STATUS_ERROR;
            goto exit;
        }
    }

    if (p2p_find_peer(getStaP2pStartGrpForm->intf, getStaP2pStartGrpForm->devId,
            getStaP2pStartGrpForm->init_go_neg) <= 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    if (getStaP2pStartGrpForm->init_go_neg) {
        ctrl = open_wpa_mon(getStaP2pStartGrpForm->intf);
        if (ctrl == NULL) {
            DPRINT_WARNING(WFA_WNG, "open wpa mon fail.\n");
            status = STATUS_ERROR;
            goto exit;
        }
    }

    wSNPRINTF(buf, sizeof(buf), "P2P_CONNECT %s %s%s%s%s go_intent=%d",
            getStaP2pStartGrpForm->devId,
            gDut.p2p_params.wps_method == WPS_METHOD_PBC ?
                    "pbc" : gDut.p2p_params.wpsPin,
            gDut.p2p_params.wps_method == WPS_METHOD_PBC ?
                    "" : (gDut.p2p_params.wps_method == WPS_METHOD_PIN_DISPLAY ?
                            " display" :
                            (gDut.p2p_params.wps_method == WPS_METHOD_PIN_LABEL ?
                                    " label" : " keypad")),
            gDut.p2p_params.persistent ? " persistent" : "",
            getStaP2pStartGrpForm->init_go_neg ? "" : " auth",
            getStaP2pStartGrpForm->intent_val);
    if (freq > 0)
        wSNPRINTF(buf + strlen(buf), sizeof(buf) - strlen(buf), " freq=%d",
                freq);

    if (wpa_command(getStaP2pStartGrpForm->intf, buf) < 0) {
        DPRINT_WARNING(WFA_WNG, "p2p connect fail\n");
        status = STATUS_ERROR;
        goto exit;
    }

    if (!getStaP2pStartGrpForm->init_go_neg) {
        goto exit;
    } else {
        int res;
        const char *events[] = {
                "P2P-GROUP-STARTED",
                "P2P-GO-NEG-FAILURE",
                "P2P-GROUP-FORMATION-FAILURE",
                NULL
        };

        res = get_wpa_cli_events(ctrl, events, buf, sizeof(buf));
        wpa_ctrl_detach(ctrl);
        wpa_ctrl_close(ctrl);
        ctrl = NULL;
        if (res < 0) {
            DPRINT_WARNING(WFA_WNG, "wait for p2p formation events fail\n");
            status = STATUS_ERROR;
            goto exit;
        }
        DPRINT_INFO(WFA_OUT, "formation event: %s\n", buf);
        if (strstr(buf, "P2P-GO-NEG-FAILURE")) {
            //For 5.1.5 DEVUT and Test Bed device have both GO Intent value 15.
            status = STATUS_COMPLETE;
            goto exit;
        } else if (strstr(buf, "P2P-GROUP-FORMATION-FAILURE")) {
            status = STATUS_ERROR;
            goto exit;
        } else {
            char *pos, *group_type, *ssid, *go_dev_addr;

            pos = strchr(buf, ' ');
            if (pos == NULL) {
                status = STATUS_ERROR;
                goto exit;
            }
            pos++;
            pos = strchr(pos, ' ');
            if (pos == NULL) {
                status = STATUS_ERROR;
                goto exit;
            }
            pos++;
            group_type = pos;
            pos = strchr(group_type, ' ');
            if (pos == NULL) {
                status = STATUS_ERROR;
                goto exit;
            }
            *pos++ = '\0';
            DPRINT_INFO(WFA_OUT, "group_type: %s\n", group_type);

            ssid = strstr(pos, "ssid=\"");
            if (ssid == NULL) {
                status = STATUS_ERROR;
                goto exit;
            }
            ssid += 6;
            pos = strchr(ssid, '"');
            if (pos == NULL) {
                status = STATUS_ERROR;
                goto exit;
            }
            *pos++ = '\0';
            DPRINT_INFO(WFA_OUT, "ssid: %s\n", ssid);

            go_dev_addr = strstr(pos, "go_dev_addr=");
            if (go_dev_addr == NULL) {
                status = STATUS_ERROR;
                goto exit;
            }
            go_dev_addr += 12;
            go_dev_addr[WFA_P2P_DEVID_LEN - 1] = '\0';
            DPRINT_INFO(WFA_OUT, "go_dev_addr: %s\n", go_dev_addr);

            wSNPRINTF(infoResp->cmdru.grpFormInfo.result,
                    sizeof(infoResp->cmdru.grpFormInfo.result),
                    "%s",
                    strcmp(group_type, "GO") == 0 ? "GO" : "CLIENT");

            wSNPRINTF(infoResp->cmdru.grpFormInfo.grpId,
                    sizeof(infoResp->cmdru.grpFormInfo.grpId),
                    "%s %s",
                    go_dev_addr, ssid);
        }
    }

exit:
    if (ctrl) {
        wpa_ctrl_detach(ctrl);
        wpa_ctrl_close(ctrl);
    }
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_P2P_START_GRP_FORMATION_RESP_TLV,
            sizeof(dutCmdResponse_t), (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return status;
}

/*
 * wfaStaP2pDissolve():
 */
int wfaStaP2pDissolve(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaP2pDissolve_t *staP2pDissolve = (caStaP2pDissolve_t *)caCmdBuf;
    enum _response_staus status = STATUS_COMPLETE;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);
#ifdef CONFIG_MTK_WFD
    wfaMtkWfdCmd_rtspTeardown();
    sleep(1);
    /* disconnect rtsp first */
#endif

    if (wpa_command(staP2pDissolve->intf, "P2P_GROUP_REMOVE *") < 0) {
        status = STATUS_ERROR;
        goto exit;
    }

exit:
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_P2P_DISSOLVE_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaSendP2pInvReq():
 */
int wfaStaSendP2pInvReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaSendP2pInvReq_t *getStaP2pInvReq= (caStaSendP2pInvReq_t *)caCmdBuf;
    enum _response_staus status = STATUS_COMPLETE;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    char strcmd[256]={0}, buf[1024] = {0};
    int res;
    struct wpa_ctrl *ctrl = NULL;

    DPRINT_INFO(WFA_OUT, "\n      PEER devId (%s) ",getStaP2pInvReq->devId);
    DPRINT_INFO(WFA_OUT, "\n      grpId      (%s) ",getStaP2pInvReq->grpId);
    DPRINT_INFO(WFA_OUT, "\n      reinvoke   (%d) \n",getStaP2pInvReq->reinvoke);

    if (p2p_find_peer(getStaP2pInvReq->intf, getStaP2pInvReq->devId, 1) <= 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    if (getStaP2pInvReq->reinvoke == 1)
    {
        //find p2p persistent network id
        int netId = 0;
        char list[4096];
        char *pos;

        if (wpa_command_resp(getStaP2pInvReq->intf, "LIST_NETWORKS", list, sizeof(list)) < 0) {
            status = STATUS_ERROR;
            goto exit;
        }
        DPRINT_INFO(WFA_OUT, "%s\n", list);

        /*
         * output format:
         * network id / ssid / bssid / flags
         * 1       DIRECT-rB7.1.2  02:08:22:fe:8b:51       [DISABLED][P2P-PERSISTENT]
         */
        pos = strstr(list, "[DISABLED][P2P-PERSISTENT]");
        if (pos == NULL) {
            status = STATUS_ERROR;
            goto exit;
        }

        while (pos > list && pos[-1] != '\n')
            pos--;
        netId = atoi(pos);

        memset(strcmd, '\0', sizeof(strcmd));
        wSNPRINTF(strcmd, sizeof(strcmd), "P2P_INVITE persistent=%d peer=%s",
                netId, getStaP2pInvReq->devId);
    }
    else
    {
        memset(strcmd, '\0', sizeof(strcmd));
        wSNPRINTF(strcmd, sizeof(strcmd), "P2P_INVITE group=%s peer=%s",
                getStaP2pInvReq->intf, getStaP2pInvReq->devId);
    }

    ctrl = open_wpa_mon(getStaP2pInvReq->intf);
    if (ctrl == NULL) {
        DPRINT_ERR(WFA_ERR, "open wpa mon fail.\n");
        status = STATUS_ERROR;
        goto exit;
    }

    if (wpa_command(getStaP2pInvReq->intf, strcmd) < 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    res = get_wpa_cli_event(ctrl, "P2P-INVITATION-RESULT", buf, sizeof(buf));

    if (res < 0) {
        DPRINT_ERR(WFA_ERR, "wfaStaSendP2pInvReq...TIMEOUT\n");
        status = STATUS_ERROR;
        goto exit;
    }

    DPRINT_INFO(WFA_OUT, "buf: %s\n", buf);

    char *tmp = NULL;
    int recvInviteRspStatus=0;

    tmp = strtok(buf," ");
    tmp = strtok(NULL," ");

    /* Handle status=0 */
    tmp = strtok(tmp,"=");
    tmp = strtok(NULL,"=");
    recvInviteRspStatus = atoi(tmp);

    if (recvInviteRspStatus!= 0) {
        DPRINT_ERR(WFA_ERR, "wfaStaSendP2pInvReq...status:%d ==>fail\n", recvInviteRspStatus);
        /* 5.1.13 Negative Scenario
         * Do not respond with STATUS_ERROR, it makes UCC stop
         */
    }
    else{
        DPRINT_INFO(WFA_OUT, "wfaStaSendP2pInvReq...status:0 ==>ok\n");
    }

exit:
    if (ctrl) {
        wpa_ctrl_detach(ctrl);
        wpa_ctrl_close(ctrl);
    }

    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_P2P_SEND_INV_REQ_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}


/*
 * wfaStaAcceptP2pInvReq():
 */
int wfaStaAcceptP2pInvReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    /* uncomment and use it
     * caStaAcceptP2pInvReq_t *getStaP2pInvReq= (caStaAcceptP2pInvReq_t *)caCmdBuf;
     */

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    // Implement the function and this does not return any thing back.

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_ACCEPT_INV_REQ_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}


/*
 * wfaStaSendP2pProvDisReq():
 */
int wfaStaSendP2pProvDisReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaSendP2pProvDisReq_t *getStaP2pProvDisReq= (caStaSendP2pProvDisReq_t *)caCmdBuf;
    char buf[256];
    char *config_method;
    enum _response_staus status = STATUS_COMPLETE;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (strcasecmp(getStaP2pProvDisReq->confMethod, "Display") == 0) {
        config_method = "display";
    } else if (strcasecmp(getStaP2pProvDisReq->confMethod, "Keypad") == 0) {
        config_method = "keypad";
    } else if (strcasecmp(getStaP2pProvDisReq->confMethod, "Label") == 0) {
        config_method = "label";
    } else if (strcasecmp(getStaP2pProvDisReq->confMethod, "PushButton") == 0) {
        config_method = "pbc";
    } else {
        status = STATUS_ERROR;
        goto exit;
    }

    if (p2p_find_peer(getStaP2pProvDisReq->intf, getStaP2pProvDisReq->devId, 0) <= 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    wSNPRINTF(buf, sizeof(buf), "P2P_PROV_DISC %s %s",
            getStaP2pProvDisReq->devId, config_method);
    if (wpa_command(getStaP2pProvDisReq->intf, buf) != 0) {
        status = STATUS_ERROR;
        goto exit;
    }

exit:
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_P2P_SEND_PROV_DIS_REQ_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}
/*
 * wfaStaSetWpsPbc():
 */
int wfaStaSetWpsPbc(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaSetWpsPbc_t *getStaSetWpsPbc= (caStaSetWpsPbc_t *)caCmdBuf;
    enum _response_staus status = STATUS_COMPLETE;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (getStaSetWpsPbc->grpid_flag) {
        if (wpa_command(getStaSetWpsPbc->intf, "WPS_PBC") < 0) {
            status = STATUS_ERROR;
            goto exit;
        }
    }
    gDut.p2p_params.wps_method = WPS_METHOD_PBC;

exit:
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_WPS_SETWPS_PBC_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaWpsReadPin():
 */
int wfaStaWpsReadPin(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaWpsReadPin_t *getStaWpsReadPin= (caStaWpsReadPin_t *)caCmdBuf;
    enum _response_staus status = STATUS_COMPLETE;
    char buf[256];

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    wSTRNCPY(gDut.p2p_params.wpsPin, "48120203", WFA_WPS_PIN_LEN);
    wSTRNCPY(infoResp->cmdru.wpsPin, gDut.p2p_params.wpsPin,
            WFA_WPS_PIN_LEN);

    if (getStaWpsReadPin->grpid_flag) {
        wSNPRINTF(buf, sizeof(buf), "WPS_PIN any %s", gDut.p2p_params.wpsPin);
        if (wpa_command(getStaWpsReadPin->intf, buf) < 0) {
            status = STATUS_ERROR;
            goto exit;
        }
    }
    gDut.p2p_params.wps_method = WPS_METHOD_PIN_DISPLAY;

exit:
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_WPS_READ_PIN_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaWpsReadLabel():
 */
int wfaStaWpsReadLabel(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaWpsReadLabel_t *getStaWpsReadLabel= (caStaWpsReadLabel_t *)caCmdBuf;
    enum _response_staus status = STATUS_COMPLETE;
    char buf[256];

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    wSTRNCPY(gDut.p2p_params.wpsPin, "48120203", WFA_WPS_PIN_LEN);
    wSTRNCPY(infoResp->cmdru.wpsPin, gDut.p2p_params.wpsPin,
            WFA_WPS_PIN_LEN);

    if (getStaWpsReadLabel->grpid_flag) {
        wSNPRINTF(buf, sizeof(buf), "WPS_PIN any %s", gDut.p2p_params.wpsPin);
        if (wpa_command(getStaWpsReadLabel->intf, buf) < 0) {
            status = STATUS_ERROR;
            goto exit;
        }
    }
    gDut.p2p_params.wps_method = WPS_METHOD_PIN_LABEL;

exit:
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_WPS_READ_PIN_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaWpsEnterPin():
 */
int wfaStaWpsEnterPin(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaWpsEnterPin_t *getStaWpsEnterPin= (caStaWpsEnterPin_t *)caCmdBuf;
    enum _response_staus status = STATUS_COMPLETE;
    char buf[256];

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    wSTRNCPY(gDut.p2p_params.wpsPin, getStaWpsEnterPin->wpsPin,
            WFA_WPS_PIN_LEN - 1);
#ifdef CONFIG_MTK_WFD
    if ((sigma_mode == SIGMA_MODE_WFD) &&
        (gDut.p2p_params.role != 1 /* not GO */)) {
        /* Do not perform WPS connect if we are not GO */
        /* or TC 6.1.3 will fail with mediatek test bed */
    }
    else
#endif
    if (getStaWpsEnterPin->grpid_flag) {
        wSNPRINTF(buf, sizeof(buf), "WPS_PIN any %s", getStaWpsEnterPin->wpsPin);
        if (wpa_command(getStaWpsEnterPin->intf, buf) < 0) {
            status = STATUS_ERROR;
            goto exit;
        }
    }
    gDut.p2p_params.wps_method = WPS_METHOD_KEYPAD;

exit:
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_WPS_ENTER_PIN_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaGetPsk():
 */
int wfaStaGetPsk(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaGetPsk_t *getStaGetPsk= (caStaGetPsk_t *)caCmdBuf;
    char passphrase[64];
    enum _response_staus status = STATUS_COMPLETE;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (wpa_command_resp(getStaGetPsk->intf, "P2P_GET_PASSPHRASE",
            passphrase, sizeof(passphrase)) < 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    wSTRNCPY(infoResp->cmdru.pskInfo.ssid, gDut.p2p_params.current_ssid,
            WFA_SSID_NAME_LEN);
    wSTRNCPY(infoResp->cmdru.pskInfo.passPhrase, passphrase, WFA_PSK_PP_LEN);

exit:
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_P2P_GET_PSK_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaP2pReset():
 */
int wfaStaP2pReset(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaWpsEnterPin_t *getStaP2pReset= (caStaWpsEnterPin_t *)caCmdBuf;
    enum _response_staus status = STATUS_COMPLETE;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    gDut.p2p_params.persistent = WFA_DISABLED;
    gDut.p2p_params.p2p_mode = P2P_MODE_IDLE;
    gDut.p2p_params.wps_method = WPS_METHOD_NONE;

    if (wpa_command(getStaP2pReset->intf, "P2P_GROUP_REMOVE *") < 0) {
        status = STATUS_ERROR;
        goto exit;
    }
    if (wpa_command(getStaP2pReset->intf, "P2P_STOP_FIND") < 0) {
        status = STATUS_ERROR;
        goto exit;
    }
    if (wpa_command(getStaP2pReset->intf, "P2P_FLUSH") < 0) {
        status = STATUS_ERROR;
        goto exit;
    }
    if (wpa_command(getStaP2pReset->intf, "P2P_SERVICE_FLUSH") < 0) {
        status = STATUS_ERROR;
        goto exit;
    }
    if (wpa_command(getStaP2pReset->intf, "P2P_SET disabled 0") < 0) {
        status = STATUS_ERROR;
        goto exit;
    }
    if (wpa_command(getStaP2pReset->intf, "P2P_SET ssid_postfix ") < 0) {
        status = STATUS_ERROR;
        goto exit;
    }
    if (wpa_command(getStaP2pReset->intf, "P2P_EXT_LISTEN") < 0) {
        status = STATUS_ERROR;
        goto exit;
    }
    if (wpa_command(getStaP2pReset->intf, "SET p2p_go_intent 7") < 0) {
        status = STATUS_ERROR;
        goto exit;
    }
    if (wpa_command(getStaP2pReset->intf, "P2P_SET go_apsd disable") < 0) {
        status = STATUS_ERROR;
        goto exit;
    }
    if (wpa_command(getStaP2pReset->intf, "SAVE_CONFIG") < 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    if (shell(DHCPRESET) != WFA_SUCCESS) {
        DPRINT_WARNING(WFA_WNG, "reset p2p dhcp fail\n");
        status = STATUS_ERROR;
        goto exit;
    }

    if (shell("ifconfig %s 0.0.0.0", getStaP2pReset->intf) != WFA_SUCCESS) {
        DPRINT_WARNING(WFA_WNG, "set p2p interface down fail\n");
        status = STATUS_ERROR;
        goto exit;
    }

    remove_p2p_persistent_networks(getStaP2pReset->intf);
    remove_wpa_networks(WFA_STAUT_IF);

exit:
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_P2P_RESET_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaGetP2pIpConfig():
 */
int wfaStaGetP2pIpConfig(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    caStaGetIpConfigResp_t *ifinfo = &(infoResp->cmdru.getIfconfig);
    caStaGetP2pIpConfig_t *staGetP2pIpConfig= (caStaGetP2pIpConfig_t *)caCmdBuf;
    enum _response_staus status = STATUS_COMPLETE;
    char ip[30];
    int count;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (strlen(staGetP2pIpConfig->grpId) == 0) {
        DPRINT_WARNING(WFA_WNG, "group id is NULL\n");
        status = STATUS_ERROR;
        goto exit;
    }

    // default timeout to get ip: 120s
    count = 120;
    while (count > 0) {
        count--;
        if (get_wpa_status(staGetP2pIpConfig->intf, "ip_address", ip,
                sizeof(ip)) == 0 && strlen(ip) > 0) {
            DPRINT_INFO(WFA_OUT, "IP address %s\n", ip);
            break;
        }
        wSLEEP(1);
    }
    if (count == 0) {
        DPRINT_WARNING(WFA_WNG, "can NOT get ip address\n");
        status = STATUS_ERROR;
        goto exit;
    }

    if (get_ip_config(staGetP2pIpConfig->intf, ifinfo) == WFA_SUCCESS) {
        status = STATUS_COMPLETE;
    } else {
        status = STATUS_ERROR;
        goto exit;
    }
    // TODO: assume p2p always uses DHCP
    ifinfo->isDhcp = 1;

    DPRINT_INFO(WFA_OUT,
            "mac_addr: %s, ip: %s, mask: %s, dns1: %s, dns2: %s, isDhcp: %d\n",
            ifinfo->mac,
            ifinfo->ipaddr,
            ifinfo->mask,
            ifinfo->dns[0],
            ifinfo->dns[1],
            ifinfo->isDhcp);

exit:
    infoResp->status = status;
    wfaEncodeTLV(WFA_STA_P2P_GET_IP_CONFIG_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaSendServiceDiscoveryReq():
 */
int wfaStaSendServiceDiscoveryReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaSendServiceDiscoveryReq_t *staSendServiceDiscoveryReq= (caStaSendServiceDiscoveryReq_t *)caCmdBuf;
    char buf[MAX_CMD_BUFF];
    enum _response_staus status = STATUS_COMPLETE;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (p2p_find_peer(staSendServiceDiscoveryReq->intf, staSendServiceDiscoveryReq->devId, 1) <= 0) {
        status = STATUS_ERROR;
        goto exit;
    }

    wSNPRINTF(buf, sizeof(buf),"P2P_SERV_DISC_REQ %s 02000001", staSendServiceDiscoveryReq->devId);
    if (wpa_command(staSendServiceDiscoveryReq->intf, buf) != 0) {
        status = STATUS_ERROR;
        goto exit;
    }

exit:
    infoResp.status = status;
    wfaEncodeTLV(WFA_STA_P2P_SEND_SERVICE_DISCOVERY_REQ_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaSendP2pPresenceReq():
 */
int wfaStaSendP2pPresenceReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_SEND_PRESENCE_REQ_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaSetSleepReq():
 */
int wfaStaSetSleepReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;
    caStaSetSleep_t *staSetSleepReq= (caStaSetSleep_t *) caCmdBuf;
    enum _response_staus status = STATUS_COMPLETE;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (wpa_command(staSetSleepReq->intf, "DRIVER p2p_set_sleep") != 0) {
        DPRINT_WARNING(WFA_WNG, "supplicant driver cmd (p2p_set_sleep) fail\n");
        status = STATUS_ERROR;
    }

    resp->status = status;
    wfaEncodeTLV(WFA_STA_P2P_SET_SLEEP_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}
/*
 * wfaStaSetOpportunisticPsReq():
 */
int wfaStaSetOpportunisticPsReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;

    printf("\n Entry wfaStaSetOpportunisticPsReq... ");
    // Implement the function and this does not return any thing back.


    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_SET_OPPORTUNISTIC_PS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}
#endif

#ifndef WFA_STA_TB
/*
 * wfaStaPresetParams():
 */

int wfaStaPresetParams(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;

    DPRINT_INFO(WFA_OUT, "Inside wfaStaPresetParameters function ...\n");

    // Implement the function and its sub commands
    infoResp.status = STATUS_COMPLETE;

    wfaEncodeTLV(WFA_STA_PRESET_PARAMETERS_RESP_TLV, 4, (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}
int wfaStaSet11n(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

    dutCmdResponse_t infoResp;
    dutCmdResponse_t *v11nParamsResp = &infoResp;

#ifdef WFA_11N_SUPPORT_ONLY

    caSta11n_t * v11nParams = (caSta11n_t *)caCmdBuf;

    int st =0; // SUCCESS

    DPRINT_INFO(WFA_OUT, "Inside wfaStaSet11n function....\n");

    if(v11nParams->addba_reject != 0xFF && v11nParams->addba_reject < 2)
    {
        // implement the funciton
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_addba_reject failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->ampdu != 0xFF && v11nParams->ampdu < 2)
    {
        // implement the funciton

        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_ampdu failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->amsdu != 0xFF && v11nParams->amsdu < 2)
    {
        // implement the funciton
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_amsdu failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->greenfield != 0xFF && v11nParams->greenfield < 2)
    {
        // implement the funciton
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "_set_greenfield failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->mcs32!= 0xFF && v11nParams->mcs32 < 2 && v11nParams->mcs_fixedrate[0] != '\0')
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_mcs failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }
    else if (v11nParams->mcs32!= 0xFF && v11nParams->mcs32 < 2 && v11nParams->mcs_fixedrate[0] == '\0')
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_mcs32 failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }
    else if (v11nParams->mcs32 == 0xFF && v11nParams->mcs_fixedrate[0] != '\0')
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_mcs32 failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->rifs_test != 0xFF && v11nParams->rifs_test < 2)
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_rifs_test failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->sgi20 != 0xFF && v11nParams->sgi20 < 2)
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_sgi20 failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->smps != 0xFFFF)
    {
        if(v11nParams->smps == 0)
        {
            // implement the funciton
            //st = wfaExecuteCLI(gCmdStr);
        }
        else if(v11nParams->smps == 1)
        {
            // implement the funciton
            //st = wfaExecuteCLI(gCmdStr);
            ;
        }
        else if(v11nParams->smps == 2)
        {
            // implement the funciton
            //st = wfaExecuteCLI(gCmdStr);
            ;
        }
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_smps failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->stbc_rx != 0xFFFF)
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_stbc_rx failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->width[0] != '\0')
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_11n_channel_width failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->_40_intolerant != 0xFF && v11nParams->_40_intolerant < 2)
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_40_intolerant failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->txsp_stream != 0 && v11nParams->txsp_stream <4)
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_txsp_stream failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }

    }

    if(v11nParams->rxsp_stream != 0 && v11nParams->rxsp_stream < 4)
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_rxsp_stream failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

#endif

    v11nParamsResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, 4, (BYTE *)v11nParamsResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
    return WFA_SUCCESS;
}
#endif

#ifdef CONFIG_MTK_P2P
/*
 * wfaStaAddArpTableEntry():
 */
int wfaStaAddArpTableEntry(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    /* caStaAddARPTableEntry_t *staAddARPTableEntry= (caStaAddARPTableEntry_t *)caCmdBuf; uncomment and use it */

    printf("\n Entry wfastaAddARPTableEntry... ");
    // Implement the function and this does not return any thing back.

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_ADD_ARP_TABLE_ENTRY_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaBlockICMPResponse():
 */
int wfaStaBlockICMPResponse(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    /* caStaBlockICMPResponse_t *staAddARPTableEntry= (caStaBlockICMPResponse_t *)caCmdBuf; uncomment and use it */

    printf("\n Entry wfaStaBlockICMPResponse... ");
    // Implement the function and this does not return any thing back.

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_BLOCK_ICMP_RESPONSE_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}
#endif
/*
 * wfaStaSetRadio():
 */

int wfaStaSetRadio(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *setRadio = (dutCommand_t *)caCmdBuf;
    dutCmdResponse_t *staCmdResp = &gGenericResp;
    caStaSetRadio_t *sr = &setRadio->cmdsu.sr;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetRadio ...\n");

    if(sr->mode == WFA_OFF)
    {
        // turn radio off
    }
    else
    {
        // always turn the radio on
    }

    staCmdResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_RADIO_RESP_TLV, 4, (BYTE *)staCmdResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaSetRFeature():
 */

int wfaStaSetRFeature(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *dutCmd = (dutCommand_t *)caCmdBuf;
    caStaRFeat_t *rfeat = &dutCmd->cmdsu.rfeat;
    dutCmdResponse_t *caResp = &gGenericResp;
    int prog = 0;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetRFeature ... prog=%s,cs=%s,uapsd=%d\n",
        rfeat->prog, rfeat->chswitchmode, rfeat->uapsd);

    prog = str2prog(rfeat->prog);
    if(prog == PROG_TYPE_TDLS) {
        int offset = 0;

        if (str_same(rfeat->secchoffset, "20"))
            offset = 0;
        else if (str_same(rfeat->secchoffset, "40above"))
            offset = 1;
        else if (str_same(rfeat->secchoffset, "40below"))
            offset = 3;

        if (rfeat->uapsd == eEnable || rfeat->uapsd == eDisable) {
            shell(IWPRIV" %s set_power_mode %d", dutCmd->intf, (int)rfeat->uapsd);
        }
        if (str_same(rfeat->chswitchmode, "Initiate")) {
            /* for gen3/gen4 */
            shell(IWPRIV" %s driver \"set_chip tdls 1 %s %d 0 %d 0 0\"",
                dutCmd->intf, rfeat->peer, rfeat->offchnum, offset);

            /* for others */
            shell(IWPRIV" %s set_str_cmd 0_9_%s_0_1_1_12_%d_%d_1",
                dutCmd->intf, rfeat->peer, rfeat->offchnum, offset);
        } else {
            /* for gen3/gen4 */
            shell(IWPRIV" %s driver \"set_chip tdls 0 %s %d 0 %d 0 0\"",
                 dutCmd->intf, rfeat->peer, rfeat->offchnum, offset);

            /* for others */
           shell(IWPRIV" %s set_str_cmd 0_9_%s_0_1_0_0_%d_%d_0",
               dutCmd->intf, rfeat->peer, rfeat->offchnum, offset);
       }
    } else if (prog == PROG_TYPE_MBO) {
        if (rfeat->chPrefClear)
            mbo_set_non_pref_chan(dutCmd->intf, NULL);

        if (rfeat->chans.chPrefNum)
            mbo_set_non_pref_chan(dutCmd->intf, &rfeat->chans);

        if (rfeat->cellularDataCap)
            mbo_set_cellular_data_cap(dutCmd->intf, rfeat->cellularDataCap);
    }
    else if (prog == PROG_TYPE_HE) {
        if (*(rfeat->ltf) != '\0') {
            DPRINT_INFO(WFA_OUT, "\nwfaStaSetRFeature HE LTF=%s\n", rfeat->ltf);
            if (str_same(rfeat->ltf, "3.2"))
                shell(IWPRIV" %s driver \"set_chip raCtrl 0x55 0xff00\"", dutCmd->intf);
            else if (str_same(rfeat->ltf, "6.4"))
                shell(IWPRIV" %s driver \"set_chip raCtrl 0x55 0xff55\"", dutCmd->intf);
            else if (str_same(rfeat->ltf, "12.8"))
                shell(IWPRIV" %s driver \"set_chip raCtrl 0x55 0xffaa\"", dutCmd->intf);
        }
        if (*(rfeat->gi) != '\0') {
            DPRINT_INFO(WFA_OUT, "\nwfaStaSetRFeature HE GI=%s\n", rfeat->gi);
            if (str_same(rfeat->gi, "0.8"))
                shell(IWPRIV" %s driver \"set_chip raCtrl 0x54 0xff00\"", dutCmd->intf);
            else if (str_same(rfeat->gi, "1.6"))
                shell(IWPRIV" %s driver \"set_chip raCtrl 0x54 0xff55\"", dutCmd->intf);
            else if (str_same(rfeat->gi, "3.2"))
                shell(IWPRIV" %s driver \"set_chip raCtrl 0x54 0xffaa\"", dutCmd->intf);
        }

        if (rfeat->oMCtrl_ULMUDisable == eDisable) {
            shell(IWPRIV" %s driver \"set_chip send_omi 0 1 0x411 1\"", dutCmd->intf);
        } else if (rfeat->oMCtrl_ULMUDisable == eEnable) {
            shell(IWPRIV" %s driver \"set_chip send_omi 0 1 0x8411 1 \"", dutCmd->intf);
        }

        /* Setup TWT */
        if (str_same(rfeat->twt_param.twtSetup, "request")) {
            shell(IWPRIV" %s driver \"SET_TWT_PARAMS 4 0 %d %d %d %d %d %d %d\"",
                dutCmd->intf,
                rfeat->twt_param.setupCommand,
                rfeat->twt_param.twtTrigger,
                rfeat->twt_param.flowType,
                rfeat->twt_param.wakeIntervalExp,
                rfeat->twt_param.protection,
                rfeat->twt_param.nominalMinWakeDur,
                rfeat->twt_param.wakeIntervalMantissa);
        } else if (str_same(rfeat->twt_param.twtSetup, "teardown")) {
            shell(IWPRIV" %s driver \"SET_TWT_PARAMS 5 %d\"",
                dutCmd->intf,
                rfeat->twt_param.flowID);
        }

        if (rfeat->txsuppdu == WFA_ENABLED) {
            setTxPPDUEnable(1);
        } else if (rfeat->txsuppdu == WFA_DISABLED) {
            setTxPPDUEnable(0);
        }

        if (str_same(rfeat->twt_param.operation, "suspend")) {
            if (rfeat->twt_param.suspendDuration) {
                shell(IWPRIV" %s driver \"SET_TWT_PARAMS 7 %d %d %d %d\"",
                    dutCmd->intf,
                    rfeat->twt_param.flowID,
                    1,
                    rfeat->twt_param.suspendDuration & 0x00000000FFFFFFFF,
                    (rfeat->twt_param.suspendDuration & 0xFFFFFFFF00000000)>>32);
            } else {
                shell(IWPRIV" %s driver \"SET_TWT_PARAMS 6 %d\"",
                    dutCmd->intf,
                    rfeat->twt_param.flowID);
            }
        } else if (str_same(rfeat->twt_param.operation, "resume")) {
            shell(IWPRIV" %s driver \"SET_TWT_PARAMS 7 %d %d %d %d\"",
                dutCmd->intf,
                rfeat->twt_param.flowID,
                1,
                rfeat->twt_param.resumeDuration & 0x00000000FFFFFFFF,
                (rfeat->twt_param.resumeDuration & 0xFFFFFFFF00000000)>>32);
        }

#ifdef CONFIG_MTK_MBO
        if (rfeat->chPrefClear)
            mbo_set_non_pref_chan(dutCmd->intf, NULL);

        if (rfeat->chans.chPrefNum)
            mbo_set_non_pref_chan(dutCmd->intf, &rfeat->chans);

        if (rfeat->cellularDataCap)
            mbo_set_cellular_data_cap(dutCmd->intf, rfeat->cellularDataCap);
#endif
    }

    caResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_RFEATURE_RESP_TLV, 4, (BYTE *)caResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaStartWfdConnection():
 */
int wfaStaStartWfdConnection(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaStartWfdConn_t *staStartWfdConn= (caStaStartWfdConn_t *)caCmdBuf; //uncomment and use it

    printf("\n Entry wfaStaStartWfdConnection... ");

#ifdef CONFIG_MTK_WFD
    char cmd[256];
    char buf[1024];
    BYTE intent;
    int result = 0;
    char sessionIdStr[WFA_WFD_SESSION_ID_LEN] = {'\0'};

    char *wfdSubElems = NULL;
    char rtspPortStr[16] = {'\0'};
    long rtspPort;
    char devInfoStr[16] = {'\0'};
    long devInfo;
    int sessionAvail = 0;
    char peerIpStr[24] = {'\0'};
    char sessionId[32] = {'\0'};
    static pthread_t wfdThreadNego;

    int localRespLen;
    dutCmdResponse_t localInfoResp;
    char localrespBuf[WFA_BUFF_4K];
    unsigned short tag;
    int ret;
    char p2pGrpId[WFA_P2P_GRP_ID_LEN] = {"\0"};
    char p2pRole[8] = {"\0"};

    char devAddr[18] = {'\0'};
    char ssid[34] = {'\0'};

    caStaP2pStartGrpForm_t staP2pStartGrpForm;

    DPRINT_INFO(WFA_OUT, "\nConnection info: inf[%s],peer[0]=[%s],peer[1]=[%s],GoIntent[%d],Init WFD[%d]\n",
    staStartWfdConn->intf,
    staStartWfdConn->peer[0],
    (strlen(staStartWfdConn->peer[1])?staStartWfdConn->peer[1]:"NULL"),
    staStartWfdConn->intent_val,
    staStartWfdConn->init_wfd);

    if (staStartWfdConn->oper_chn_flag)
    {
        DPRINT_INFO(WFA_OUT, "set op channel: %d\n", staStartWfdConn->oper_chn);
        wSNPRINTF(cmd, sizeof(cmd), "SET p2p_oper_channel %d",
                staStartWfdConn->oper_chn);
        if (wpa_command(staStartWfdConn->intf, cmd) != 0)
        {
            DPRINT_ERR(WFA_ERR, "set op channel fail!!\n");
        }
    }

    if (staStartWfdConn->intent_val_flag)
    {
        intent = staStartWfdConn->intent_val;
        if (intent == 16)
        {
        //default intent value
            intent = 7;
        }

        DPRINT_INFO(WFA_OUT, "set Go Intent: %d\n", intent);
        wSNPRINTF(cmd, sizeof(cmd), "SET p2p_go_intent %d", intent);
        if (wpa_command(staStartWfdConn->intf, cmd) != 0)
        {
            DPRINT_ERR(WFA_ERR, "set Go Intent fail!!\n");
        }
    }

    // Prepare for groud formation
    memset((void *)&staP2pStartGrpForm, 0, sizeof(caStaP2pStartGrpForm_t));
    strncpy(staP2pStartGrpForm.intf, staStartWfdConn->intf, WFA_IF_NAME_LEN-1);
    staP2pStartGrpForm.intf[WFA_IF_NAME_LEN-1] = '\0';
    strncpy(staP2pStartGrpForm.devId, staStartWfdConn->peer[0], WFA_P2P_DEVID_LEN-1);
    staP2pStartGrpForm.devId[WFA_P2P_DEVID_LEN-1] = '\0';
    staP2pStartGrpForm.intent_val = (staStartWfdConn->intent_val == 16) ? 7 : staStartWfdConn->intent_val;
    staP2pStartGrpForm.init_go_neg = 1;
    staP2pStartGrpForm.oper_chn = staStartWfdConn->oper_chn;
    staP2pStartGrpForm.oper_chn_flag = staStartWfdConn->oper_chn_flag;
    staP2pStartGrpForm.ssid_flag = 0;

    // get WFD information()
    if (staStartWfdConn->init_wfd)
    {
        memset(cmd, '\0', sizeof(cmd));
        wSNPRINTF(cmd, sizeof(cmd), "P2P_PEER %s", staStartWfdConn->peer[0]);
        if (wpa_command_resp(staStartWfdConn->intf, cmd, buf, sizeof(buf)) < 0)
        {
            DPRINT_ERR(WFA_ERR, "Run cmd \'%s\' fail!!\n", cmd);
        }

        if (strncmp(buf, "FAIL", 4) == 0)
        {
            DPRINT_ERR(WFA_ERR, "Unable to find peer %s!!\n", staStartWfdConn->peer[0]);
            result = -1;
            goto Exit;
        }

        wfdSubElems = strstr(buf, "wfd_subelems=");
        if (strlen(wfdSubElems) == 0)
        {
          DPRINT_ERR(WFA_ERR, "Unable to find wfd_subelems in peer %s!!\n", staStartWfdConn->peer[0]);
          result = -1;
          goto Exit;
        }

        wfdSubElems = strstr(wfdSubElems, "=") + 1;
        DPRINT_INFO(WFA_OUT, "wfd_subelems: %s\n", wfdSubElems);

        #define OFFSET_OF_RTSTPORT 10
        memcpy(rtspPortStr, wfdSubElems + OFFSET_OF_RTSTPORT, 4);
        rtspPort = strtol(rtspPortStr, NULL, 16);
        DPRINT_INFO(WFA_OUT, "wfd rtspPort = %d\n", rtspPort);

        #define OFFSET_OF_DEVINFO 6
        memcpy(devInfoStr, wfdSubElems + OFFSET_OF_DEVINFO, 4);
        devInfo = strtol(devInfoStr, NULL, 16);
        DPRINT_INFO(WFA_OUT, "wfd defInfo = %02x\n", devInfo);

        sessionAvail = (devInfo & 0x30) > 1;
        DPRINT_INFO(WFA_OUT, "wfd session available = %d\n", sessionAvail);

        if (!sessionAvail)
        {
            DPRINT_ERR(WFA_ERR, "wfd session is not available!!\n");
            result = -1;
            goto Exit;
        }

        DPRINT_INFO(WFA_OUT, "Starting P2P connection to peer[0]=[%s]\n", staStartWfdConn->peer[0]);

        memset((void *)localrespBuf, 0, WFA_BUFF_4K);
        memset((void *)&localInfoResp, 0, sizeof(dutCmdResponse_t));
        ret = wfaStaP2pStartGrpFormation(sizeof(caStaP2pStartGrpForm_t), (BYTE *)&staP2pStartGrpForm,
                                         &localRespLen, (BYTE *)&localrespBuf[0]);

        if ((ret == STATUS_COMPLETE) &&
            (wfaGetTLVvalue(sizeof(dutCmdResponse_t), (BYTE *)&localrespBuf[0], &localInfoResp) == WFA_SUCCESS) &&
            strlen(localInfoResp.cmdru.grpFormInfo.grpId) != 0)
        {

            DPRINT_INFO(WFA_OUT, "P2P formation Success!!\n");

            sscanf(localInfoResp.cmdru.grpFormInfo.grpId, "%[0-9a-z:] %s", devAddr, ssid);

            DPRINT_INFO(WFA_OUT, "devAddr: %s\n", devAddr);
            DPRINT_INFO(WFA_OUT, "ssid: %s\n", ssid);

            snprintf(p2pGrpId,sizeof(p2pGrpId), "%s %s", devAddr, ssid);
            DPRINT_INFO(WFA_OUT, "P2P connected to Addr: %s, SSID:%s\n", devAddr,ssid);
            DPRINT_INFO(WFA_OUT, "I'm P2P %s\n", localInfoResp.cmdru.grpFormInfo.result);

            result = mtkWfdP2pConnect(&staStartWfdConn->peer[0], &sessionIdStr[0], WFA_WFD_SESSION_ID_LEN);
       }
       else
       {
            DPRINT_INFO(WFA_OUT, "P2P formation Fail!!\n");
            result = -1;
       }
Exit:

        if (result != 0)
        {
            strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "NULL");
            strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
            strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
        }
        else
        {
            strcpy(&infoResp.cmdru.wfdConnInfo.result[0], &localInfoResp.cmdru.grpFormInfo.result[0]);
            strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], &localInfoResp.cmdru.grpFormInfo.grpId[0]);
            strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], &sessionIdStr[0]);
        }
    }
    else
    {
        DPRINT_INFO(WFA_OUT, "Create a thread for WFD Nego connection!!\n");
        pthread_create(&wfdThreadNego, NULL, &wfaStaWaitingWfdConnection_Nego, &staP2pStartGrpForm);

        strcpy(&infoResp.cmdru.wfdConnInfo.result[0], " ");
        strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
        strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], " ");
    }

#else

    // Fetch the GrpId and WFD session and return
    strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "1234567890");
    strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], "WIFI_DISPLAY");
    strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "GO");
#endif
    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_START_WFD_CONNECTION_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}
/*
 * wfaStaCliCommand():
 */
#ifdef _FREERTOS
int wfaStaCliCommand(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    printf("\nEntry wfaStaCliCommand; command Received: %s\n",caCmdBuf);
    DPRINT_ERR_HDR();
    printf("Exit from wfaStaCliCommand\n");
    return TRUE;
}

#else
int wfaStaCliCommand(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    char cmdName[32];
    char *pcmdStr=NULL, *str;
    int  st = 1;
    char CmdStr[WFA_CMD_STR_SZ];
    FILE *wfaCliFd;
    char wfaCliBuff[64];
    char retstr[256];
    int CmdReturnFlag =0;
    char tmp[256];
    FILE * sh_pipe;
    caStaCliCmdResp_t infoResp;

    printf("\nEntry wfaStaCliCommand; command Received: %s\n",caCmdBuf);
    memcpy(cmdName, strtok_r((char *)caCmdBuf, ",", (char **)&pcmdStr), 32);
    sprintf(CmdStr, "%s",cmdName);

    for(;;)
    {
        // construct CLI standard cmd string
        str = strtok_r(NULL, ",", &pcmdStr);
        if(str == NULL || str[0] == '\0')
            break;
        else
        {
            sprintf(gCmdStr, "%s /%s",CmdStr,str);
            str = strtok_r(NULL, ",", &pcmdStr);
            sprintf(CmdStr, "%s %s",gCmdStr,str);
        }
    }
    // check the return process
    wfaCliFd=fopen("/etc/WfaEndpoint/wfa_cli.txt","r");
    if(wfaCliFd!= NULL)
    {
        while(fgets(wfaCliBuff, 64, wfaCliFd) != NULL)
        {
            //printf("\nLine read from CLI file : %s",wfaCliBuff);
            if(ferror(wfaCliFd))
                break;

            str=strtok(wfaCliBuff,"-");
            if(strcmp(str,cmdName) == 0)
            {
                str=strtok(NULL,",");
                if (str != NULL)
                {
                    if(strcmp(str,"TRUE") == 0)
                        CmdReturnFlag =1;
                }
                else
                    printf("ERR wfa_cli.txt, inside line format not end with , or missing TRUE/FALSE\n");
                break;
            }
        }
        fclose(wfaCliFd);
    }
    else
    {
        printf("/etc/WfaEndpoint/wfa_cli.txt is not exist\n");
        goto cleanup;
    }

    //printf("\n Command Return Flag : %d",CmdReturnFlag);
    memset(&retstr[0],'\0',255);
    memset(&tmp[0],'\0',255);
    sprintf(gCmdStr, "%s",  CmdStr);
    printf("\nCLI Command -- %s\n", gCmdStr);

    sh_pipe = popen(gCmdStr,"r");
    if(!sh_pipe)
    {
        printf ("Error in opening pipe\n");
        goto cleanup;
    }

    sleep(5);
    //tmp_val=getdelim(&retstr,255,"\n",sh_pipe);
    if (fgets(&retstr[0], 255, sh_pipe) == NULL)
    {
        printf("Getting NULL string in popen return\n");
        goto cleanup;
    }
    else
        printf("popen return str=%s\n",retstr);

    sleep(2);
    if(pclose(sh_pipe) == -1)
    {
        printf("Error in closing shell cmd pipe\n");
        goto cleanup;
    }
    sleep(2);

    // find status first in output
    str = strtok_r((char *)retstr, "-", (char **)&pcmdStr);
    if (str != NULL)
    {
        memset(tmp, 0, 10);
        memcpy(tmp, str,  2);
        printf("cli status=%s\n",tmp);
        if(strlen(tmp) > 0)
            st = atoi(tmp);
        else printf("Missing status code\n");
    }
    else
    {
        printf("wfaStaCliCommand no return code found\n");
    }
    infoResp.resFlag=CmdReturnFlag;

cleanup:

    switch(st)
    {
    case 0:
        infoResp.status = STATUS_COMPLETE;
        if (CmdReturnFlag)
        {
            if((pcmdStr != NULL) && (strlen(pcmdStr) > 0) )
            {
                memset(&(infoResp.result[0]),'\0',WFA_CLI_CMD_RESP_LEN-1);
                strncpy(&infoResp.result[0], pcmdStr ,(strlen(pcmdStr) < WFA_CLI_CMD_RESP_LEN ) ? strlen(pcmdStr) : (WFA_CLI_CMD_RESP_LEN-2) );
                printf("Return CLI result string to CA=%s\n", &(infoResp.result[0]));
            }
            else
            {
                strcpy(&infoResp.result[0], "No return string found\n");
            }
        }
        break;
    case 1:
        infoResp.status = STATUS_ERROR;
        break;
    case 2:
        infoResp.status = STATUS_INVALID;
        break;
    }

    wfaEncodeTLV(WFA_STA_CLI_CMD_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    printf("Exit from wfaStaCliCommand\n");
    return TRUE;

}
#endif
/*
 * wfaStaConnectGoStartWfd():
 */

int wfaStaConnectGoStartWfd(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaConnectGoStartWfd_t *staConnecGoStartWfd= (caStaConnectGoStartWfd_t *)caCmdBuf; //uncomment and use it

    printf("\n Entry wfaStaConnectGoStartWfd... \n");

    // connect the specified GO and then establish the wfd session
#ifdef CONFIG_MTK_WFD
    caStaP2pConnect_t staP2pConnect;
    int result = 0;

    int localRespLen = 0;;
    dutCmdResponse_t localInfoResp = {0};
    char localrespBuf[WFA_BUFF_4K] = {0};
    int ret;

    char sessionIdStr[WFA_WFD_SESSION_ID_LEN] = {'\0'};

    memset((void *)&staP2pConnect, 0, sizeof(caStaP2pConnect_t));

    strncpy(staP2pConnect.intf, staConnecGoStartWfd->intf, WFA_IF_NAME_LEN - 1);
    staP2pConnect.intf[WFA_IF_NAME_LEN - 1] = '\0';

    strncpy(staP2pConnect.grpid, staConnecGoStartWfd->grpid, WFA_P2P_GRP_ID_LEN - 1);
    staP2pConnect.grpid[WFA_P2P_GRP_ID_LEN - 1] = '\0';

    strncpy(staP2pConnect.devId, staConnecGoStartWfd->devId, WFA_P2P_DEVID_LEN - 1);
    staP2pConnect.devId[WFA_P2P_DEVID_LEN - 1] = '\0';

    // connect to the GO
    DPRINT_INFO(WFA_OUT, "Starting P2P connection, Inf[%s] GroupId[%s] DevId[%s]\n"
        ,&staP2pConnect.intf[0] ,&staP2pConnect.grpid[0], &staP2pConnect.devId[0]);

    ret = wfaStaP2pConnect(sizeof(caStaP2pConnect_t) ,(BYTE *)&staP2pConnect ,&localRespLen ,(BYTE *)&localrespBuf[0]);

    if (ret != STATUS_COMPLETE
            || ((wfaGetTLVvalue(sizeof(dutCmdResponse_t), (BYTE *)&localrespBuf[0], &localInfoResp) != WFA_SUCCESS)
            || localInfoResp.status != STATUS_COMPLETE))
    {
        DPRINT_ERR(WFA_ERR, "P2P connection...Fail!!\n");
        result = -1;
        goto Exit;

    }

    // establish the wfd session

    result= mtkWfdP2pConnect(staConnecGoStartWfd->devId, &sessionIdStr[0], WFA_WFD_SESSION_ID_LEN);

Exit:
    if (result != 0)
    {
        infoResp.status = STATUS_COMPLETE;
        strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "NULL");
        strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
        strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
    }
    else
    {
        infoResp.status = STATUS_COMPLETE;
        strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "CLIENT");
        strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], staConnecGoStartWfd->grpid);
        strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], sessionIdStr);
    }
#else
    // Fetch WFD session and return
    strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "1234567890");
#endif

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_CONNECT_GO_START_WFD_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaGenerateEvent():
 */

int wfaStaGenerateEvent(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaGenEvent_t *staGenerateEvent= (caStaGenEvent_t *)caCmdBuf; //uncomment and use it
    caWfdStaGenEvent_t *wfdGenEvent;

#ifdef CONFIG_MTK_WFD
    int evtType = 0;
    int hid_type;
#endif

    printf("\n Entry wfaStaGenerateEvent... ");


    // Geneate the specified action and return with complete/error.
    if(staGenerateEvent->program == PROG_TYPE_WFD)
    {
        wfdGenEvent = &staGenerateEvent->wfdEvent;
        if(wfdGenEvent ->type == eUibcGen)
        {
#ifdef CONFIG_MTK_WFD
#define SEND_EVENT_COUNT    3

            DPRINT_ERR(WFA_ERR, "%s: UibcGen, evt type = %d\n", __FUNCTION__, wfdGenEvent->wfdUibcEventType);

            if (wfdGenEvent->wfdUibcEventType == eSingleTouchEvent ||
                    wfdGenEvent->wfdUibcEventType == eMouseEvent ||
                    wfdGenEvent->wfdUibcEventType == eMultiTouchEvent ||
                    wfdGenEvent->wfdUibcEventType == eKeyBoardEvent)
            {
                evtType = wfdGenEvent->wfdUibcEventType;
            }
            else
            {
                DPRINT_ERR(WFA_ERR, "%s: Unknown uibc event type(%d), treat as single touch event!\n",
                        __FUNCTION__, wfdGenEvent->wfdUibcEventType);
                wfdGenEvent->wfdUibcEventType = eSingleTouchEvent;
            }

            //for (count = 0; count < SEND_EVENT_COUNT; count ++)
            //{
            if (evtType == 0)
                DPRINT_ERR(WFA_ERR, "evtType: 0\n");

            if (wfaMtkWfdCmd_rtspUibcGenEvent(evtType) != 0) {
                DPRINT_ERR(WFA_ERR, "UibcGenEvent Failed!\n");
            } else {
                DPRINT_ERR(WFA_ERR, "UibcGenEvent Success!\n");
	    }
            //    usleep(200000);
            //}
#endif
        }
        else if(wfdGenEvent ->type == eUibcHid)
        {
#ifdef CONFIG_MTK_WFD

            if (wfdGenEvent->wfdUibcEventType == eKeyBoardEvent)
                hid_type = eKeyBoardEvent;
            else if (wfdGenEvent->wfdUibcEventType == eMouseEvent)
                hid_type = eMouseEvent;
            else
            {
                DPRINT_ERR(WFA_ERR, "%s: !!Unsupported uibc event type(%d), treat as HID Keyboard event!\n",
                        __FUNCTION__, wfdGenEvent->wfdUibcEventType);
                hid_type = eKeyBoardEvent;
            }

            if (wfaMtkWfdCmd_rtspUibcHidcEvent(hid_type) != 0) {
                DPRINT_ERR(WFA_ERR, "UibcHidcEvent Failed!\n");
            } else {
                DPRINT_ERR(WFA_ERR, "UibcHidcEvent Success!\n");
	    }
#endif
        }
        else if(wfdGenEvent ->type == eFrameSkip)
        {

        }
        else if(wfdGenEvent ->type == eI2cRead)
        {
        }
        else if(wfdGenEvent ->type == eI2cWrite)
        {
        }
        else if(wfdGenEvent ->type == eInputContent)
        {
        }
        else if(wfdGenEvent ->type == eIdrReq)
        {
#ifdef CONFIG_MTK_WFD
            DPRINT_ERR(WFA_ERR, "%s: GenerateEvent = SendIdrReq\n", __FUNCTION__);

            if (wfaMtkWfdCmd_rtspSendIdrReq() != 0)
                DPRINT_ERR(WFA_ERR, "SendIdrReq Failed!\n");
#endif
        }
    }

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_GENERATE_EVENT_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}




/*
 * wfaStaReinvokeWfdSession():
 */

int wfaStaReinvokeWfdSession(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
//  caStaReinvokeWfdSession_t *staReinvokeSession= (caStaReinvokeWfdSession_t *)caCmdBuf; //uncomment and use it

    printf("\n Entry wfaStaReinvokeWfdSession... ");

    // Reinvoke the WFD session by accepting the p2p invitation   or sending p2p invitation


    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_REINVOKE_WFD_SESSION_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}


int wfaStaGetParameter(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaGetParameter_t *staGetParam= (caStaGetParameter_t *)caCmdBuf; //uncomment and use it


    caStaGetParameterResp_t *paramList = &infoResp.cmdru.getParamValue;

    printf("\n Entry wfaStaGetParameter... ");
#ifdef CONFIG_MTK_WFD
    char addr[128] = {0}, cmd[64];
    char *devListBuf = (char *)&paramList->devList;
    int devListBufLen = sizeof(paramList->devList);
    char *pos, *end;
    int length;

#endif

    // Check the program type
    if(staGetParam->program == PROG_TYPE_WFD)
    {
        if(staGetParam->getParamValue == eDiscoveredDevList )
        {
            // Get the discovered devices, make space seperated list and return, check list is not bigger than 128 bytes.
            paramList->getParamType = eDiscoveredDevList;
#ifdef CONFIG_MTK_WFD

            pos = devListBuf;
            end = devListBuf + devListBufLen;

            if(wpa_command_resp(staGetParam->intf, "P2P_PEER FIRST", addr, sizeof(addr)) >= 0)
            {
                // the first 16 bytes is mac address
                addr[17] = '\0';
                strncpy(devListBuf, addr, devListBufLen);
                devListBuf[devListBufLen - 1] = '\0';
                pos += strlen(devListBuf);
                snprintf(cmd, sizeof(cmd), "P2P_PEER NEXT-%s", addr);
                memset(addr, 0, sizeof(addr));
                while (wpa_command_resp(staGetParam->intf, cmd, addr, sizeof(addr)) >= 0)
                {
                    if (memcmp(addr, "FAIL", 4) == 0)
                    {
                        break;
                    }
                    addr[17] = '\0';
                    length = snprintf(pos, end - pos, " %s", addr);
                    if (length < 0 || length >= end -pos)
                    {
                        break;
                    }
                    pos += length;
                    snprintf(cmd, sizeof(cmd), "P2P_PEER NEXT-%s", addr);
                    memset(addr, 0, sizeof(addr));
                }
            }
            DPRINT_INFO(WFA_OUT, "p2p_peers: %s\n", devListBuf);
#else
            strcpy((char *)&paramList->devList, "11:22:33:44:55:66 22:33:44:55:66:77 33:44:55:66:77:88");
#endif
        }
    }

    if(staGetParam->program == PROG_TYPE_WFDS)
    {

        if(staGetParam->getParamValue == eDiscoveredDevList )
        {
            // Get the discovered devices, make space seperated list and return, check list is not bigger than 128 bytes.
            paramList->getParamType = eDiscoveredDevList;
            strcpy((char *)&paramList->devList, "11:22:33:44:55:66 22:33:44:55:66:77 33:44:55:66:77:88");

        }
        if(staGetParam->getParamValue == eOpenPorts)
        {
            // Run the port checker tool
            // Get all the open ports and make space seperated list and return, check list is not bigger than 128 bytes.
            paramList->getParamType = eOpenPorts;
            strcpy((char *)&paramList->devList, "22 139 445 68 9700");

        }

    }
    if(staGetParam->program == PROG_TYPE_NAN)
    {
      if(staGetParam->getParamValue == eMasterPref )
      {
          // Get the master preference of the device and return the value
          paramList->getParamType = eMasterPref;
          strcpy((char *)&paramList->masterPref, "0xff");
      }
    }
#ifdef CONFIG_MTK_HE
    if (staGetParam->program == PROG_TYPE_HE)
    {
        if (staGetParam->getParamValue == eRssi)
        {
            // Get the master preference of the device and return the value
            paramList->getParamType = eRssi;
#ifndef _FREERTOS
            shell(IWPRIV" %s driver \"stat\" | grep Beacon | cut -f2 -d'=' | cut -f2 -d' '", staGetParam->intf);
            wSTRNCPY(paramList->masterPref, gDut.shellResult, sizeof(paramList->masterPref) - 1);

            shell(MTKINBANDCMD" %s %s", staGetParam->intf, MTK_INBAND_TXOP_ENHANCE);
#else
            shell(IWPRIV" %s driver \"stat\"", staGetParam->intf);
            memset(paramList->masterPref, 0, sizeof(paramList->masterPref));
            wSTRNCPY(paramList->masterPref, g_rssi, strlen(g_rssi));

            if ((strcmp("HE-5.61.1_24G", gDut.ssid) == 0 || strcmp("HE-5.61.1", gDut.ssid) == 0) &&
                ++gDut.trafficIteration == 4)
                shell(IWPRIV" %s driver \"set_mcr 0x820E2028 0x04040404\"", staGetParam->intf);
#endif
        }

        if (staGetParam->getParamValue == ePMK)
        {
            //TODO
            DPRINT_INFO(WFA_OUT, "get pmk NOT support yet\n");
        }
    }
#endif

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_GET_PARAMETER_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}


int wfaStaNfcAction(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "Entering %s(), Not support now, return error.\n", __func__);

    resp->status = STATUS_ERROR;
    wfaEncodeTLV(WFA_STA_NFC_ACTION_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaStaExecAction(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

    dutCmdResponse_t infoResp;
    caStaExecAction_t *staExecAction = (caStaExecAction_t *)caCmdBuf;  //comment if not used

     printf("\n Entry wfaStaExecAction... ");

    if(staExecAction->prog == PROG_TYPE_NAN)
    {
        // Perform necessary configurations and actions
        // return the MAC address conditionally as per CAPI specification
    }

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_EXEC_ACTION_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}

int wfaStaInvokeCommand(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

    dutCmdResponse_t infoResp;
    caStaInvokeCmd_t *staInvokeCmd = (caStaInvokeCmd_t *)caCmdBuf;  //uncomment and use it

     printf("\n Entry wfaStaInvokeCommand... ");


     // based on the command type , invoke API or complete the required procedures
     // return the  defined parameters based on the command that is received ( example response below)

    if(staInvokeCmd->cmdType == ePrimitiveCmdType && staInvokeCmd->InvokeCmds.primtiveType.PrimType == eCmdPrimTypeAdvt )
    {
         infoResp.cmdru.staInvokeCmd.invokeCmdRspType = eCmdPrimTypeAdvt;
         infoResp.cmdru.staInvokeCmd.invokeCmdResp.advRsp.numServInfo = 1;
         strcpy(infoResp.cmdru.staInvokeCmd.invokeCmdResp.advRsp.servAdvInfo[0].servName,"org.wi-fi.wfds.send.rx");
         infoResp.cmdru.staInvokeCmd.invokeCmdResp.advRsp.servAdvInfo[0].advtID = 0x0000f;
         strcpy(infoResp.cmdru.staInvokeCmd.invokeCmdResp.advRsp.servAdvInfo[0].serviceMac,"ab:cd:ef:gh:ij:kl");
    }
    else if (staInvokeCmd->cmdType == ePrimitiveCmdType && staInvokeCmd->InvokeCmds.primtiveType.PrimType == eCmdPrimTypeSeek)
    {
        infoResp.cmdru.staInvokeCmd.invokeCmdRspType = eCmdPrimTypeSeek;
        infoResp.cmdru.staInvokeCmd.invokeCmdResp.seekRsp.searchID = 0x000ff;
    }
    else if (staInvokeCmd->cmdType == ePrimitiveCmdType && staInvokeCmd->InvokeCmds.primtiveType.PrimType == eCmdPrimTypeConnSession)
    {
        infoResp.cmdru.staInvokeCmd.invokeCmdRspType = eCmdPrimTypeConnSession;
        infoResp.cmdru.staInvokeCmd.invokeCmdResp.connSessResp.sessionID = 0x000ff;
        strcpy(infoResp.cmdru.staInvokeCmd.invokeCmdResp.connSessResp.result,"GO");
        strcpy(infoResp.cmdru.staInvokeCmd.invokeCmdResp.connSessResp.grpId,"DIRECT-AB WFADUT");

    }
    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_INVOKE_CMD_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}


int wfaStaManageService(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

    dutCmdResponse_t infoResp;
    //caStaMngServ_t *staMngServ = (caStaMngServ_t *)caCmdBuf;  //uncomment and use it

     printf("\n Entry wfaStaManageService... ");

    // based on the manage service type , invoke API's or complete the required procedures
    // return the  defined parameters based on the command that is received ( example response below)
    strcpy(infoResp.cmdru.staManageServ.result, "CLIENT");
    strcpy(infoResp.cmdru.staManageServ.grpId, "AA:BB:CC:DD:EE:FF_DIRECT-SSID");
    infoResp.cmdru.staManageServ.sessionID = 0x000ff;

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_MANAGE_SERVICE_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}



int wfaStaGetEvents(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

    dutCmdResponse_t infoResp;
    caStaGetEvents_t *staGetEvents = (caStaGetEvents_t *)caCmdBuf;  //uncomment and use it

     printf("\n Entry wfaStaGetEvents... ");

     if(staGetEvents->program == PROG_TYPE_NAN)
    {
        // Get all the events from the Log file or stored events
        // return the  received/recorded event details - eventName, remoteInstanceID, localInstanceID, mac
    }

    // Get all the event from the Log file or stored events
    // return the  received/recorded events as space seperated list   ( example response below)
    strcpy(infoResp.cmdru.staGetEvents.result, "SearchResult SearchTerminated AdvertiseStatus SessionRequest ConnectStatus SessionStatus PortStatus");

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_GET_EVENTS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}

int wfaStaGetEventDetails(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

    dutCmdResponse_t infoResp;
    caStaGetEventDetails_t *getStaGetEventDetails = (caStaGetEventDetails_t *)caCmdBuf;  //uncomment and use it

     printf("\n Entry wfaStaGetEventDetails... ");


     // based on the Requested Event type
     // return the latest corresponding evnet detailed parameters  ( example response below)

    if(getStaGetEventDetails->eventId== eSearchResult )
    {
        // fetch from log file or event history for the search result event and return the parameters
        infoResp.cmdru.staGetEventDetails.eventID= eSearchResult;

        infoResp.cmdru.staGetEventDetails.getEventDetails.searchResult.searchID = 0x00abcd;
        strcpy(infoResp.cmdru.staGetEventDetails.getEventDetails.searchResult.serviceMac,"ab:cd:ef:gh:ij:kl");
        infoResp.cmdru.staGetEventDetails.getEventDetails.searchResult.advID = 0x00dcba;
        strcpy(infoResp.cmdru.staGetEventDetails.getEventDetails.searchResult.serviceName,"org.wi-fi.wfds.send.rx");

        infoResp.cmdru.staGetEventDetails.getEventDetails.searchResult.serviceStatus = eServiceAvilable;
    }
    else if (getStaGetEventDetails->eventId == eSearchTerminated)
    {       // fetch from log file or event history for the search terminated event and return the parameters
        infoResp.cmdru.staGetEventDetails.eventID= eSearchTerminated;
        infoResp.cmdru.staGetEventDetails.getEventDetails.searchTerminated.searchID = 0x00abcd;
    }
    else if (getStaGetEventDetails->eventId == eAdvertiseStatus)
    {// fetch from log file or event history for the Advertise Status event and return the parameters
        infoResp.cmdru.staGetEventDetails.eventID= eAdvertiseStatus;
        infoResp.cmdru.staGetEventDetails.getEventDetails.advStatus.advID = 0x00dcba;

        infoResp.cmdru.staGetEventDetails.getEventDetails.advStatus.status = eAdvertised;
    }
    else if (getStaGetEventDetails->eventId == eSessionRequest)
    {// fetch from log file or event history for the session request event and return the parameters
        infoResp.cmdru.staGetEventDetails.eventID= eSessionRequest;
        infoResp.cmdru.staGetEventDetails.getEventDetails.sessionReq.advID = 0x00dcba;
        strcpy(infoResp.cmdru.staGetEventDetails.getEventDetails.sessionReq.sessionMac,"ab:cd:ef:gh:ij:kl");
        infoResp.cmdru.staGetEventDetails.getEventDetails.sessionReq.sessionID = 0x00baba;
    }
    else if (getStaGetEventDetails->eventId ==eSessionStatus )
    {// fetch from log file or event history for the session status event and return the parameters
        infoResp.cmdru.staGetEventDetails.eventID= eSessionStatus;
        infoResp.cmdru.staGetEventDetails.getEventDetails.sessionStatus.sessionID = 0x00baba;
        strcpy(infoResp.cmdru.staGetEventDetails.getEventDetails.sessionStatus.sessionMac,"ab:cd:ef:gh:ij:kl");
        infoResp.cmdru.staGetEventDetails.getEventDetails.sessionStatus.state = eSessionStateOpen;
    }
    else if (getStaGetEventDetails->eventId == eConnectStatus)
    {
        infoResp.cmdru.staGetEventDetails.eventID= eConnectStatus;
        infoResp.cmdru.staGetEventDetails.getEventDetails.connStatus.sessionID = 0x00baba;
        strcpy(infoResp.cmdru.staGetEventDetails.getEventDetails.connStatus.sessionMac,"ab:cd:ef:gh:ij:kl");
        infoResp.cmdru.staGetEventDetails.getEventDetails.connStatus.status = eGroupFormationComplete;

    }
    else if (getStaGetEventDetails->eventId == ePortStatus)
    {
        infoResp.cmdru.staGetEventDetails.eventID= ePortStatus;
        infoResp.cmdru.staGetEventDetails.getEventDetails.portStatus.sessionID = 0x00baba;
        strcpy(infoResp.cmdru.staGetEventDetails.getEventDetails.portStatus.sessionMac,"ab:cd:ef:gh:ij:kl");
        infoResp.cmdru.staGetEventDetails.getEventDetails.portStatus.port = 1009;
        infoResp.cmdru.staGetEventDetails.getEventDetails.portStatus.status = eLocalPortAllowed;
    }



    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_GET_EVENT_DETAILS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}

void output_ssid_bssid(char* dst, int len, char* buf)
{
    char *pos, *head, *end, *substr;
    int row = 0;

    head = dst;
    end = head + len;

    /* Skip the first line (header) */
    pos = strchr(buf, '\n') + 1;
    substr = strtok(pos, "\n");
    do {
        char bssid[WFA_MAC_ADDR_STR_LEN] = {0};
        char ssid[WFA_SSID_NAME_LEN] = {0};

        memcpy(bssid, substr, WFA_MAC_ADDR_STR_LEN - 1);
        pos = strrchr(substr, '\t') + 1;
        wSNPRINTF(ssid, WFA_SSID_NAME_LEN - 1, "%s", pos);
        head += wSNPRINTF(head, end - head, ",SSID,%s,BSSID,%s", ssid, bssid);
        row++;
        substr = strtok(NULL, "\n");
    } while(substr && row < 10 && end - head > 0 );
}

int wfaStaScan(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaScan_t *staScan= (caStaScan_t *)caCmdBuf;
    dutCmdResponse_t *staScanResp = &gGenericResp;
    char buf[4096];
    char *bssid = NULL, *ssid = NULL, *freq = NULL;
    char ssid_hex[65], freq_buf[100];

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaScan ...\n");

    if ((staScan->hessid[0] != '\0')){
        wSNPRINTF(buf, sizeof(buf), "SET hessid %s", staScan->hessid);
        if (wpa_command(staScan->intf, buf) < 0) {
            staScanResp->status = STATUS_ERROR;
            goto done;
        }
    }

    if ((staScan->accs_net_type[0] != '\0')){
        wSNPRINTF(buf, sizeof(buf), "SET access_network_type %s", staScan->accs_net_type);
        if (wpa_command(staScan->intf, buf) < 0) {
            staScanResp->status = STATUS_ERROR;
            goto done;
        }
    }

    if ((staScan->ssid[0] != '\0') && strcasecmp(staScan->ssid, "ZeroLength") != 0) {
        if (2 * strlen(staScan->ssid) >= sizeof(ssid_hex)) {
            staScanResp->status = STATUS_INVALID;
            goto done;
        }
        ssid = staScan->ssid;
        ascii2hexstr(ssid, ssid_hex);
    }

    if ((staScan->bssid[0] != '\0')){
        bssid = staScan->bssid;
    }

    if ((staScan->shortSsid != 0)) {
        int shortSsidTemp;

        int2BuffBigEndian(staScan->shortSsid, (char *)shortSsidTemp);
        wSPRINTF(buf, "VENDOR_ELEM_ADD 14 FF053A%X", shortSsidTemp);
        if (wpa_command(staScan->intf, buf) < 0) {
            staScanResp->status = STATUS_ERROR;
            goto done;
        }
    }

    if (staScan->chnlFreq) {
        wSPRINTF(freq_buf, "%d", staScan->chnlFreq);
        freq = freq_buf;
    }

    wSNPRINTF(buf, sizeof(buf), "SCAN%s%s%s%s%s%s",
        bssid ? " bssid=": "", bssid ? bssid : "",
        ssid ? " ssid " : "",  ssid ? ssid_hex : "",
        freq ? " freq=" : "",  freq ? freq : "");

    if (wpa_command(staScan->intf, buf) < 0) {
        staScanResp->status = STATUS_ERROR;
        goto done;
    }

    if (staScan->output_bssid_ssid == 1) {
        memset(staScanResp->cmdru.scanResult, 0, WFA_SCAN_RESULT_LEN);
        sta_mon_event(staScan->intf, "CTRL-EVENT-SCAN");

        /* get scan result */
        if (wpa_command_resp(staScan->intf, "SCAN_RESULTS", buf, sizeof(buf)) < 0)
        {
            staScanResp->status = STATUS_ERROR;
            goto done;
        }

        output_ssid_bssid(staScanResp->cmdru.scanResult, WFA_SCAN_RESULT_LEN, buf);
    }

    staScanResp->status = STATUS_COMPLETE;

done:
    wfaEncodeTLV(WFA_STA_SCAN_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)staScanResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

#ifdef CONFIG_MTK_HE
int wfaStaSetPowerSave(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaPowerSave_t *staPs = (caStaPowerSave_t *)caCmdBuf;
    dutCmdResponse_t *caResp = &gGenericResp;
    int prog = 0;

    DPRINT_INFO(WFA_OUT, "\nEntering wfaStaSetPowerSave ...\n");

    prog = str2prog(staPs->prog);
    if (prog == PROG_TYPE_HE) {
        if(*(staPs->pwrsave) != '\0') {
            if (str_same(staPs->pwrsave, "on")) {
                /* Use for HE power save on ex.HE-5.60.1 (TWT) and HE-5.72.1 */
                shell(IWPRIV" %s set_power_mode 0", staPs->intf);
                shell(IWPRIV" %s set_power_mode 2", staPs->intf);
            }
        }
    }

    caResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_POWER_SAVE_RESP_TLV, 4, (BYTE *)caResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}
#endif

/**
 * Internal utilities
 */

int ascii2hexstr(const char *str, char *hex)
{
    int i, length;

    length = strlen(str);

    for (i = 0; i < length; i++)
        snprintf(hex + i * 2, 3, "%X", str[i]);

    hex[length * 2] = '\0';
    return 1;
}

int channel2freq(char channel)
{
    if (channel > 0 && channel <= 14)
        return channel * 5 + 2407;
    if (channel >= 36 && channel <= 161)
        return 5000 + 5 * channel;
    return 0;
}

const char* prog2str(int prog) {
    int size = sizeof(prog_str_tbl) / sizeof(char*);

    if (prog < size && prog > 0)
        return prog_str_tbl[prog];

    return prog_str_tbl[0];
}

int str2prog(const char *str) {
    int size = sizeof(prog_str_tbl) / sizeof(char*);
    int i;

    for (i = 1; i < size; i++) {
        if (str_same(str, prog_str_tbl[i]))
            return i;
    }

    return 0;
}

char* encpType2str(int encpType) {
    if (encpType == ENCRYPT_TKIP)
        return "tkip";
    else if (encpType == ENCRYPT_AESCCMP)
        return "aes-ccmp";
    else if (encpType == ENCRYPT_AESCCMP_TKIP)
        return "aes-ccmp-tkip";

    DPRINT_ERR(WFA_ERR, "Unknown encpType=%d\n", encpType);
    return "UNKNOWN";
}

int str2encpType(const char *str) {

    if(str_same(str, "tkip"))
        return ENCRYPT_TKIP;
    else if(str_same(str, "aes-ccmp"))
        return ENCRYPT_AESCCMP;
    else if (str_same(str, "aes-ccmp-tkip"))
        return ENCRYPT_AESCCMP_TKIP;

    return ENCRYPT_NONE;
}

#ifdef _FREERTOS
int chk_exist(const char *path)
{
    DPRINT_ERR_HDR();
    return 0;
}

int shell(const char *fmt, ...)
{
    va_list params;
    char *token[20];
    int token_num = 0;
    char *src, *dst;

    va_start(params, fmt);
    vsnprintf(gCmdStr, sizeof(gCmdStr), fmt, params);
    va_end(params);

    DPRINT_INFO(WFA_OUT, "shell: %s\n", gCmdStr);

    /* Remove quotation mark for cli_tokens */
    for (src = dst = gCmdStr; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != '\"')
            dst++;
    }
    *dst = '\0';

    *token = strtok(gCmdStr, " ");
    while (*(token + token_num) != NULL) {
        //DPRINT_INFO(WFA_OUT, "token: %d %s ", token_num, *(token + token_num));
        token_num++;
        *(token + token_num) = strtok(NULL, " ");
    }

    DPRINT_INFO(WFA_OUT, "token num = %d\n", token_num);
    if(strncasecmp("iwpriv", token[0], strlen("iwpriv")) == 0
#if !defined(CONFIG_CTRL_IFACE)
	|| strncasecmp("wpa_cli", token[0], strlen("wpa_cli")) == 0
#endif
      )
	cli_tokens(token_num, token);
    else {
	DPRINT_INFO(WFA_OUT, "%s: cmd not support!!check if need.\n", __func__);
	int i = 0;
	for( i = 0; i < token_num; i++)
		DPRINT_INFO(WFA_OUT, "%s ", *(token + i));
	DPRINT_INFO(WFA_OUT, "\n");
    }
    return WFA_SUCCESS;
}

#else
int chk_exist(const char *path)
{
    if (path && access(path, F_OK) != -1) {
        DPRINT_INFO(WFA_OUT, "%s exist.\n", path);
        return 1;
    } else {
        DPRINT_ERR(WFA_ERR, "%s doesn't exist!!\n", path);
        return 0;
    }
}

void wfa_cs_init(void)
{
    const char *path[] = {IWPRIV, DHCPCLIENT, DHCPSERVER, DHCPRESET, DHCPGETSERVERIP,DHCPGETCLIENTIP, SETIPCONFIG, GETIPCONFIG,
            GETPID, GETPSTATS, STOPPING, UPDATEPID, WFAPING, WFAPING6};
    int count = sizeof(path) / sizeof(*path);
    int i;
    caStaSetIpConfig_t *ipconfig = &gDut.sta_params.ipconfig;

    DPRINT_INFO(WFA_OUT, "\nEntering wfa_cs_init ...\n");

    wMEMSET(&gDut, 0, sizeof(gDut));
    ipconfig->isDhcp = 1;
    wSPRINTF(ipconfig->intf, WFA_STAUT_IF);

    /*
     * check toolkits before use them
     */
    for (i = 0; i < count; i++) {
        chk_exist(path[i]);
    }

    if (shell("cat "BIN_DIR"/testMode.txt | sed s/[[:space:]]//g") == WFA_SUCCESS) {
        DPRINT_INFO(WFA_OUT, "testMode=%s\n", gDut.shellResult);
        count = sizeof(sigma_mode_tbl) / sizeof(*sigma_mode_tbl);
        for (i = 0; i < count; i++) {
            if (str_same(gDut.shellResult, sigma_mode_tbl[i])) {
                sigma_mode = i;
                break;
            }
        }
    }

    init_driver_gen();
#ifdef CONFIG_MTK_AP
    init_ap_params();
#endif
#ifdef CONFIG_MTK_P2P
    init_p2p_params();
#endif
#ifdef CONFIG_MTK_WFD
    DPRINT_INFO(WFA_OUT, "Sigma_mode = %d\n", sigma_mode);
    if (sigma_mode == SIGMA_MODE_WFD) {
        DPRINT_INFO(WFA_OUT, "sigma_mode = SIGMA_MODE_WFD\n");
        if (wfaMtkWfdInit() != 0)
        {
            DPRINT_ERR(WFA_ERR, "Failed to init MTK WFD...\n");
            exit(1);
        }
    }
#endif

}

int shell(const char *fmt, ...)
{
    va_list params;
    FILE *filep;
    size_t len = 0;
    size_t remain_len = sizeof(gDut.shellResult);
    char *result = gDut.shellResult;

    va_start(params, fmt);
    vsnprintf(gCmdStr, sizeof(gCmdStr), fmt, params);
    va_end(params);

    DPRINT_INFO(WFA_OUT, "shell: %s\n", gCmdStr);

    if((filep=popen(gCmdStr, "r")) != NULL)
    {
        wMEMSET(gDut.shellResult, 0, remain_len);
        while (fgets(result, remain_len, filep) != NULL)
        {
            len = strlen(result);
            result += len;
            remain_len -= len;
        }
        pclose(filep);
        while ((--result >= gDut.shellResult) && (*result == '\n' || *result == '\r'));
        *(result + 1) = 0;
    } else {
        DPRINT_ERR(WFA_ERR, "shell popen error");
        return WFA_ERROR;
    }
    return WFA_SUCCESS;
}
#endif

int check_connection(const char *ifname)
{
    char result[32];
    int ret;

    ret = get_wpa_status(ifname, "wpa_state", result, sizeof(result));
    if (ret >= 0) {
        DPRINT_INFO(WFA_OUT, "wpa_state=%s\n", result);
        if (wSTRNCMP(result, "COMPLETED", 9) == 0)
            return 1;
    } else {
        DPRINT_ERR(WFA_ERR, "get_wpa_status field=\"wpa_state\" ret=%d\n", ret);
    }
    return 0;
}

void update_ip_config(caStaSetIpConfig_t *ipconfig)
{
    caStaSetIpConfig_t *cfg = &gDut.sta_params.ipconfig;
    int connected = 0;
    char* intf;

    DPRINT_INFO(WFA_OUT, "inside update_ip_config ...\n");

    // copy if new ipconfig
    if (ipconfig)
        wMEMCPY(cfg, ipconfig, sizeof(caStaSetIpConfig_t));

    intf = get_sta_intf(cfg->intf);

    // get connection status
    connected = check_connection(intf);

    // Set ipconfig after association
    if(cfg->isDhcp) {
        if (connected) {
            shell(DHCPCLIENT" %s", intf);
            DPRINT_INFO(WFA_OUT, "dhcpcd Request DHCP IP DONE!\n");
        }
    } else {
        shell(SETIPCONFIG" %s %s %s %s %s", intf,
            cfg->ipaddr, cfg->mask, cfg->pri_dns, cfg->sec_dns);
        if(cfg->defGateway[0] != '\0') {
            shell("ip route add default gw %s", cfg->defGateway);
        }
    }
}

void fixed_rate_config(const char* intf, int isconnect)
{
    int *bcc_mode;
    int *mcs_fixedrate;
    int *mcs_fixedrate_rate;

    bcc_mode = get_param_val("bcc_mode");
    mcs_fixedrate = get_param_val("mcs_fixedrate");
    mcs_fixedrate_rate = get_param_val("mcs_fixedrate_rate");

    if (bcc_mode && *bcc_mode) {
        DPRINT_INFO(WFA_OUT, "set BCC mode\n");
        shell(IWPRIV" %s driver \"set_chip raCtrl 0x51 0xff00\"", intf);
    }

    if (mcs_fixedrate && *mcs_fixedrate) {
        DPRINT_INFO(WFA_OUT, "set MCS %d\n", *mcs_fixedrate_rate);
        shell(IWPRIV" %s driver \"set_chip raCtrl 0x4F 0xff%02x\"", intf, *mcs_fixedrate_rate);
    }
#if 1
    if (bcc_mode && *bcc_mode && mcs_fixedrate && *mcs_fixedrate) {
        if (isconnect)
            /*Shell Command*/
            shell(IWPRIV" %s driver \"fixedrate=%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d\"",
                intf,
#ifndef _FREERTOS
                1,/*wlan-id*/
                8,/*Phy Mode*/
                0,/*BW*/
                *mcs_fixedrate_rate,/*Rate*/
                2,/*Nss*/
                0,/*GI*/
                0,/*Preamble*/
                0,/*STBC*/
                0,/*LDPC*/
                0,/*SPE_EN*/
                1,/*HE-LTF*/
                0,/*HE-ER-DCM*/
                0/*HE-ER-106*/);
#else
                1,/*wlan-id*/
                8,/*Phy Mode*/
                0,/*BW*/
                *mcs_fixedrate_rate,/*Rate*/
                1,/*Nss*/
                1,/*GI*/
                0,/*Preamble*/
                0,/*STBC*/
                0,/*LDPC*/
                0,/*SPE_EN*/
                1,/*HE-LTF*/
                0,/*HE-ER-DCM*/
                0/*HE-ER-106*/);
#endif
        else
            DPRINT_INFO(WFA_OUT, "\n not connected, failed to use fixedrate command\n");
    }
#endif
}

void remove_wpa_networks(const char *ifname)
{
    char buf[4096];
    char cmd[256];
    char *pos;

    DPRINT_INFO(WFA_OUT, "inside remove_wpa_networks ...\n");

    if (wpa_command_resp(ifname, "LIST_NETWORKS", buf, sizeof(buf)) < 0)
        return;

    /* Skip the first line (header) */
    pos = strchr(buf, '\n');
    if (pos == NULL)
        return;
    pos++;
    while (pos && pos[0]) {
        int id = atoi(pos);
        snprintf(cmd, sizeof(cmd), "REMOVE_NETWORK %d", id);
        wpa_command(ifname, cmd);
        pos = strchr(pos, '\n');
        if (pos)
            pos++;
    }
}

int find_network(const char *ifname, const char *ssid)
{
    char list[4096];
    char *pos;

    DPRINT_INFO(WFA_OUT, "inside find_network ... ssid=%s\n", ssid);

    if (wpa_command_resp(ifname, "LIST_NETWORKS", list, sizeof(list)) < 0)
        return -1;
    DPRINT_INFO(WFA_OUT, "%s", list);
    pos = strstr(list, ssid);
    if (!pos || pos == list || pos[strlen(ssid)] != '\t')
        return -1;

    while (pos > list && pos[-1] != '\n')
        pos--;
    gDut.networkId = atoi(pos);
    wSPRINTF(gDut.ssid, "%s", ssid);
    return 0;
}

void reset_param() {
    wMEMSET(&gDut.sta_params, 0, sizeof(gDut.sta_params));
    gDut.sta_params.type[0] = UNSET_VALUE;
    gDut.sta_params.pmf[0] = WFA_INVALID_BOOL;
    gDut.sta_params.akmSuiteType[0] = UNSET_VALUE;
    gDut.sta_params.owe[0] = UNSET_VALUE;
    gDut.sta_params.pmksaCaching[0] = UNSET_VALUE;
    gDut.sta_params.profile[0] = UNSET_VALUE;
    gDut.sta_params.peapVersion[0] = UNSET_VALUE;
    gDut.sta_params.prefer[0] = UNSET_VALUE;
    gDut.sta_params.saePwe[0] = WFA_INVALID_BOOL;
}

#ifdef _FREERTOS
/* conflicting types in wpa_supplicant 2.10 */
void *sigma_get_param(const char* name)
#else
void *get_param(const char* name)
#endif
{
    sta_params_t *params = &gDut.sta_params;

#define GET_PARAM_V(x, v) \
    if (str_same(name, #x)) { \
        if (params->x[0] != v) \
            return params->x; \
        else \
            return NULL; \
    }

#define GET_PARAM(x) GET_PARAM_V(x, 0)

    GET_PARAM(intf);
    GET_PARAM_V(type, UNSET_VALUE);
    GET_PARAM(ssid);
    GET_PARAM(keyMgmtType);
    GET_PARAM(encpType);
    GET_PARAM_V(pmf, WFA_INVALID_BOOL);
    GET_PARAM_V(peapVersion, UNSET_VALUE);
    GET_PARAM_V(akmSuiteType, UNSET_VALUE);
    GET_PARAM(clientCertificate);
    GET_PARAM(certType);
    GET_PARAM(ecGroupID);
    GET_PARAM(groupCipher);
    GET_PARAM(groupMgntCipher);
    GET_PARAM(innerEAP);
    GET_PARAM(invalidSAEElement);
    GET_PARAM(networkMode);
    GET_PARAM_V(owe, UNSET_VALUE);
    GET_PARAM(pacFile);
    GET_PARAM(pairwiseCipher);
    GET_PARAM(passphrase);
    GET_PARAM(password);
    GET_PARAM_V(pmksaCaching, UNSET_VALUE);
    GET_PARAM_V(profile, UNSET_VALUE);
    GET_PARAM(prog);
    GET_PARAM(trustedRootCA);
    GET_PARAM(tlsCipher);
    GET_PARAM(username);
    GET_PARAM_V(peapVersion, UNSET_VALUE);
    GET_PARAM_V(prefer, UNSET_VALUE);
    GET_PARAM(micAlg);
    GET_PARAM(key1);
    GET_PARAM(key2);
    GET_PARAM(key3);
    GET_PARAM(key4);
    GET_PARAM(activeKeyIdx);
    GET_PARAM(bcc_mode);
    GET_PARAM(mcs_fixedrate);
    GET_PARAM(mcs_fixedrate_rate);
    GET_PARAM(bw_sgnl);
    GET_PARAM_V(saePwe, WFA_INVALID_BOOL);

    //DPRINT_ERR(WFA_ERR, "Can't get_param %s\n", name);

#undef GET_PARAM
    return NULL;
}

char *get_param_str(const char* name) {
#ifdef _FREERTOS
        return (char*) sigma_get_param(name);
#else
        return (char*) get_param(name);
#endif
}

int *get_param_val(const char* name) {
#ifdef _FREERTOS
        return (int*) sigma_get_param(name);
#else
        return (int*) get_param(name);
#endif
}

void set_param(const char* name, void *value, unsigned int size)
{
    sta_params_t *params = &gDut.sta_params;

#define SET_PARAM(x) \
        if (str_same(name, #x) && size <= sizeof(params->x)) { \
            wMEMCPY(params->x, value, size); \
            return; \
        }

    SET_PARAM(intf);
    SET_PARAM(type);
    SET_PARAM(ssid);
    SET_PARAM(keyMgmtType);
    SET_PARAM(encpType);
    SET_PARAM(pmf);
    SET_PARAM(peapVersion);
    SET_PARAM(akmSuiteType);
    SET_PARAM(clientCertificate);
    SET_PARAM(certType);
    SET_PARAM(ecGroupID);
    SET_PARAM(groupCipher);
    SET_PARAM(groupMgntCipher);
    SET_PARAM(innerEAP);
    SET_PARAM(invalidSAEElement);
    SET_PARAM(networkMode);
    SET_PARAM(owe);
    SET_PARAM(pacFile);
    SET_PARAM(pairwiseCipher);
    SET_PARAM(passphrase);
    SET_PARAM(password);
    SET_PARAM(pmksaCaching);
    SET_PARAM(profile);
    SET_PARAM(prog);
    SET_PARAM(trustedRootCA);
    SET_PARAM(tlsCipher);
    SET_PARAM(username);
    SET_PARAM(peapVersion);
    SET_PARAM(prefer);
    SET_PARAM(micAlg);
    SET_PARAM(key1);
    SET_PARAM(key2);
    SET_PARAM(key3);
    SET_PARAM(key4);
    SET_PARAM(activeKeyIdx);
    SET_PARAM(bcc_mode);
    SET_PARAM(mcs_fixedrate);
    SET_PARAM(mcs_fixedrate_rate);
    SET_PARAM(bw_sgnl);
    SET_PARAM(saePwe);

    DPRINT_ERR(WFA_ERR, "Can't set_param %s\n", name);

#undef SET_PARAM
}

void set_param_val(const char* name, int value) {
    set_param(name, &value, sizeof(int));
}

void set_param_str(const char* name, char *value) {
    //    DPRINT_INFO(WFA_OUT, "set_param_str %s=[%s] %d\n", name, value, strlen(value));
    set_param(name, value, strlen(value) + 1);
}

int add_network_common(const char *ifname)
{
    const char *ssid = get_param_str("ssid");
    int id;
    const char *val;

    if (ssid == NULL) {
        DPRINT_ERR(WFA_ERR, "add_network error!! ifname=%s...\n", ifname);
        return -2;
    }

    // remove all networks first
    //remove_wpa_networks(ifname);

    id = add_network(ifname);
    if (id < 0) {
        DPRINT_ERR(WFA_ERR, "add_network error!! ifname=%s ...\n", ifname);
        return -2;
    }

    if (set_network_quoted(ifname, id, "ssid", ssid) < 0) {
        DPRINT_ERR(WFA_ERR, "add_network error!! ssid=%s ...\n", ssid);
        return -2;
    }

    gDut.networkId = id;
    wSPRINTF(gDut.ssid, "%s", ssid);

    if (gDut.program == PROG_TYPE_HE || gDut.program == PROG_TYPE_TDLS) {
#ifdef _FREERTOS
        if (strcmp("HE-5.64.1_24G", gDut.ssid) == 0)
            shell(IWPRIV" %s driver \"SET_BA_SIZE 2\"", ifname);
        else if (strcmp("HE-5.64.1", gDut.ssid) == 0)
            shell(IWPRIV" %s driver \"SET_BA_SIZE 4\"", ifname);
        else /* Reset to default ba size */
            shell(IWPRIV" %s driver \"SET_BA_SIZE 16\"", ifname);
#else
        shell(MTKINBANDCMD" %s %s", ifname, MTK_INBAND_BASIZE_ADJUST);
#endif
        shell(MTKINBANDCMD" %s %s", ifname, MTK_INBAND_SET_FWLOG);
        shell(MTKINBANDCMD" %s %s", ifname, MTK_INBAND_DISABLE_BACKGROUND_SCAN);
   }

    val = get_param_str("prog");
    if (str_same(val, "hs2")) {
        char buf[100];

        wSPRINTF(buf,  "ENABLE_NETWORK %d no-connect", id);
        wpa_command(ifname, buf);

        val = (const char *)get_param_val("prefer");
        if (val && *val > 0)
            set_network(ifname, id, "priority", "1");
    }

    DPRINT_INFO(WFA_OUT, "add_network_common id=%d ssid=%s ...\n", id, ssid);
    return id;
}

int str_same(const char* str1, const char* str2)
{
    if (str1 && str2) {
        return strcasecmp(str1, str2) == 0;
    }
    return 0;
}

int set_wpa_common(const char* intf, const int id)
{
    const char *val;
    int *type;
    int owe, suite_b = 0;
    int ret = STATUS_COMPLETE;
    char buf[256];

    type = get_param_val("type");
    owe = type && *type == SEC_TYPE_OWE;

    val = get_param_str("keyMgmtType");
    if (!val && owe)
        val = "OWE";
    if (val == NULL) {
        DPRINT_INFO(WFA_OUT, "\nset_wpa_common: keyMgmtType is null...\n");
    } else if (!str_same(val, "OWE")) {
        // keyMgmtType to proto
        if (str_same(val, "wpa") || str_same(val, "wpa-psk"))
            wSPRINTF(buf, "WPA");
        else if (str_same(val, "wpa2-wpa-psk") || str_same(val, "wpa2-wpa-ent"))
            wSPRINTF(buf, "WPA WPA2");
        else
            wSPRINTF(buf, "WPA2");
        if (str_same(val, "SuiteB"))
            suite_b = 1;
        if (set_network(intf, id, "proto", buf) < 0) {
            ret = STATUS_ERROR;
        }
    }

    val = get_param_str("encpType");
    if (val) {
        // encrptype to pairwise
        if (str_same(val, "tkip")) {
            wSPRINTF(buf, "TKIP");
        } else if (str_same(val, "aes-ccmp")) {
            wSPRINTF(buf, "CCMP");
        } else if (str_same(val, "aes-ccmp-tkip")) {
            wSPRINTF(buf, "CCMP TKIP");
        } else if (str_same(val, "aes-gcmp")) {
            wSPRINTF(buf, "GCMP");
            if (set_network(intf, id, "group", buf) < 0) {
                ret = STATUS_ERROR;
            }
        }
        if (set_network(intf, id, "pairwise", buf) < 0)
            ret = STATUS_ERROR;
    }

    val = get_param_str("PairwiseCipher");
    if (val) {
        if (str_same(val, "AES-GCMP-256"))
            wSPRINTF(buf, "GCMP-256");
        else if (str_same(val, "AES-CCMP-256"))
            wSPRINTF(buf, "CCMP-256");
        else if (str_same(val, "AES-GCMP-128"))
            wSPRINTF(buf, "GCMP");
        else if (str_same(val, "AES-CCMP-128"))
            wSPRINTF(buf, "CCMP");
        if (set_network(intf, id, "pairwise", buf) < 0)
            ret = STATUS_ERROR;
    }

    val = get_param_str("GroupCipher");
    if (val) {
        if (str_same(val, "AES-GCMP-256"))
            wSPRINTF(buf, "GCMP-256");
        else if (str_same(val, "AES-CCMP-256"))
            wSPRINTF(buf, "CCMP-256");
        else if (str_same(val, "AES-GCMP-128"))
            wSPRINTF(buf, "GCMP");
        else if (str_same(val, "AES-CCMP-128"))
            wSPRINTF(buf, "CCMP");
        if (set_network(intf, id, "group", buf) < 0)
            ret = STATUS_ERROR;
    }

    val = get_param_str("GroupMgntCipher");
    if (val) {
        if (str_same(val, "BIP-GMAC-256"))
            wSPRINTF(buf, "BIP-GMAC-256");
        else if (str_same(val, "BIP-CMAC-256"))
            wSPRINTF(buf, "BIP-CMAC-256");
        else if (str_same(val, "BIP-GMAC-128"))
            wSPRINTF(buf, "BIP-GMAC-128");
        else if (str_same(val, "BIP-CMAC-128"))
            wSPRINTF(buf, "AES-128-CMAC");
        if (set_network(intf, id, "group_mgmt", buf) < 0)
            ret = STATUS_ERROR;
    }

    gDut.sta_pmf = WFA_DISABLED;

    val = (const char *)get_param_val("pmf");
    if (val) {
        int pmf = *val;

        if (pmf == WFA_DISABLED || pmf == WFA_F_DISABLED)
            gDut.sta_pmf = WFA_DISABLED;
        else if (pmf == WFA_REQUIRED || pmf == WFA_F_REQUIRED || owe || suite_b)
            gDut.sta_pmf = WFA_REQUIRED;
        else
            gDut.sta_pmf = WFA_OPTIONAL;

        wSPRINTF(buf, "%d", gDut.sta_pmf);
        if (set_network(intf, id, "ieee80211w", buf) < 0)
            ret = STATUS_ERROR;
    }
    return ret;
}

int set_eap_common(const char* intf, const int id)
{
    char buf[256];
    char *ext = NULL;
    const char *val, *alg;
    const int *akm;

    if (set_wpa_common(intf, id) != STATUS_COMPLETE)
        return STATUS_ERROR;

    val = get_param_str("keyMgmtType");
    alg = get_param_str("micAlg");
    akm = get_param_val("AKMSuiteType");

    buf[0] = '\0';

    // key_mgmt
    if (str_same(val, "SuiteB")) {
        wSPRINTF(buf, "WPA-EAP-SUITE-B-192");
    } else if (str_same(alg, "SHA-256")) {
        wSPRINTF(buf, "WPA-EAP-SHA256");
    } else if (str_same(alg, "SHA-1")) {
        wSPRINTF(buf, "WPA-EAP");
    } else if (str_same(val, "wpa2-ft")) {
        wSPRINTF(buf, "FT-EAP");
    } else if (str_same(val, "wpa2-sha256") ||
           gDut.sta_pmf == WFA_REQUIRED || gDut.sta_pmf == WFA_OPTIONAL) {
        wSPRINTF(buf, "WPA-EAP WPA-EAP-SHA256");
    } else if (akm && *akm == 14) {
        if (gDut.sta_pmf == WFA_REQUIRED || gDut.sta_pmf == WFA_OPTIONAL)
            wSPRINTF(buf, "WPA-EAP-SHA256 FILS-SHA256");
        else
            wSPRINTF(buf, "WPA-EAP FILS-SHA256");
        if (set_network(intf, id, "erp", "1") < 0)
             return STATUS_ERROR;
    } else if (akm && *akm == 15) {
        if (gDut.sta_pmf == WFA_REQUIRED || gDut.sta_pmf == WFA_OPTIONAL)
            wSPRINTF(buf, "WPA-EAP-SHA256 FILS-SHA384");
        else
            wSPRINTF(buf, "WPA-EAP FILS-SHA384");
        if (set_network(intf, id, "erp", "1") < 0)
             return STATUS_ERROR;
    } else if (gDut.sta_pmf == WFA_OPTIONAL) {
        wSPRINTF(buf, "WPA-EAP WPA-EAP-SHA256");
    } else {
        wSPRINTF(buf, "WPA-EAP");
    }
    if (set_network(intf, id, "key_mgmt", buf) < 0)
        return STATUS_ERROR;


    val = get_param_str("trustedRootCA");
    if (val) {
        ext = strstr(val, ".pem");
        if (ext)
            *ext = 0;
        wSPRINTF(buf, EAP_CERT_PATH"/%s.pem", val);
        chk_exist(buf);
        if (set_network_quoted(intf, id, "ca_cert", buf) < 0)
            return STATUS_ERROR;
    }

    val = get_param_str("clientCertificate");
    if (val) {
        ext = strstr(val, ".pem");
        if (ext)
            *ext = 0;

        wSPRINTF(buf, EAP_CERT_PATH"/%s.pem", val);
        chk_exist(buf);
        if (set_network_quoted(intf, id, "client_cert", buf) < 0)
            return STATUS_ERROR;

        wSPRINTF(buf, EAP_CERT_PATH"/%s.key", val);
        chk_exist(buf);
        if (set_network_quoted(intf, id, "private_key", buf) < 0)
            return STATUS_ERROR;
    }

    val = get_param_str("username");
    if (val) {
        if (set_network_quoted(intf, id, "identity", val) < 0)
            return STATUS_ERROR;
    }

    val = get_param_str("password");
    if (val) {
        if (set_network_quoted(intf, id, "password", val) < 0)
            return STATUS_ERROR;
    }

    // default enable okc
    if (set_network(intf, id, "proactive_key_caching", "1") < 0)
        return STATUS_ERROR;

    return STATUS_COMPLETE;
}

int sta_set_open(int *respLen, BYTE *respBuf) {
    char *intf = get_param_str("intf");
    dutCmdResponse_t *setOpenResp = &gGenericResp;
    int id;

    id = add_network_common(intf);
    if (id < 0) {
        setOpenResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_network(intf, id, "key_mgmt", "NONE") < 0) {
        setOpenResp->status = STATUS_ERROR;
        goto done;
    }

    setOpenResp->status = STATUS_COMPLETE;
done:
    wfaEncodeTLV(WFA_STA_SET_SECURITY_RESP_TLV, 4, (BYTE *)setOpenResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}


int sta_set_owe(int *respLen, BYTE *respBuf)
{
    char *intf = get_param_str("intf");
    dutCmdResponse_t *setOweResp = &gGenericResp;
    const char *val;
    int id;

    id = add_network_common(intf);
    if (id < 0) {
        setOweResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_wpa_common(intf, id) != STATUS_COMPLETE) {
        setOweResp->status = STATUS_ERROR;
        goto done;
    }

    if (set_network(intf, id, "key_mgmt", "OWE") < 0) {
        setOweResp->status = STATUS_ERROR;
        goto done;
    }

    val = get_param_str("ECGroupID");
    if (val) {
        if (set_network(intf, id, "owe_group", val) < 0) {
            setOweResp->status = STATUS_ERROR;
            goto done;
        }
    }

    val = (const char *)get_param_val("pmf");
    if (!val) {
        gDut.sta_pmf = WFA_REQUIRED;
        if (set_network(intf, id, "ieee80211w", "2") < 0) {
            setOweResp->status = STATUS_ERROR;
            goto done;
        }
    }

    setOweResp->status = STATUS_COMPLETE;
done:
    wfaEncodeTLV(WFA_STA_SET_SECURITY_RESP_TLV, 4, (BYTE *)setOweResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int sta_mon_event(const char* ifname, const char* event) {

    struct wpa_ctrl *ctrl;
    int res;
    char buf[1024];

    DPRINT_INFO(WFA_OUT, "\nEntering sta_mon_event ...\n");

    /* wait for connected event */
    ctrl = open_wpa_mon(ifname);
    if (ctrl == NULL) {
        DPRINT_ERR(WFA_ERR, "Failed to open wpa_supplicant monitor %s\n", event);
        return -1;
    }

    res = get_wpa_cli_event(ctrl, event, buf, sizeof(buf));

    wpa_ctrl_detach(ctrl);
    wpa_ctrl_close(ctrl);

    if (res < 0)
        DPRINT_WARNING(WFA_WNG, "Failed to get %s from network %d on %s\n", event, gDut.networkId, ifname);

    return res;
}

static void init_driver_gen()
{
    if (shell("getprop ro.vendor.wlan.gen") != WFA_SUCCESS)
        return;
    if (strstr(gDut.shellResult, "gen2"))
        gDut.driver_gen = GEN_2;
    else if (strstr(gDut.shellResult, "gen3"))
        gDut.driver_gen = GEN_3;
    else if (strstr(gDut.shellResult, "gen4m"))
        gDut.driver_gen = GEN_4m;
    else if (strstr(gDut.shellResult, "gen4"))
        gDut.driver_gen = GEN_4;
    else
        gDut.driver_gen = GEN_UNKNOWN;
    DPRINT_INFO(WFA_OUT, "propertyValue=%s, driver gen=%d.\n",
            gDut.shellResult,
            gDut.driver_gen);
}

#ifdef _FREERTOS
int set_sigma_mode(int sigma_mode) {

    int ret = -1;
    DPRINT_WARNING(WFA_OUT, "%s: sigma mode: %d\n", __func__, sigma_mode);

    if (sigma_mode == SIGMA_MODE_TG_N) { /* TGn */
	ret = shell(IWPRIV" wlan0 driver set_chip sigma 6");
    }
    else if (sigma_mode == SIGMA_MODE_VOE) { /* VOE */
	ret = shell(IWPRIV" wlan0 driver set_chip sigma D");
    }
    else if (sigma_mode == SIGMA_MODE_TG_AX) {/* AX */
	ret = shell(IWPRIV" wlan0 driver set_chip sigma E");
    }
    else { /* AC as default*/
	ret = shell(IWPRIV" wlan0 driver set_chip sigma 1");
    }
    DPRINT_WARNING(WFA_OUT, "%s: ret: %d\n", __func__, ret);
    return ret;
}
#endif

#ifdef CONFIG_MTK_AP
static void config_hostapd_he(FILE *f, ap_params_t *p_ap_params)
{
    int ht40plus = 0, ht40minus = 0;

    if (f == NULL || p_ap_params == NULL) {
        DPRINT_WARNING(WFA_WNG, "invalid args\n");
        return;
    }

    if (p_ap_params->mode != AP_MODE_11ax &&
        p_ap_params->program != PROGRAME_TYPE_HE)
        return;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    fprintf(f, "ieee80211n=1\n");
    fprintf(f, "ieee80211ax=1\n");

    if (p_ap_params->channel <= 14) {
        /* 2.4G */
        fprintf(f, "hw_mode=g\n");

        if (p_ap_params->ch_width == CHANNEL_WIDTH_40) {
            if (p_ap_params->ch_offset == CHANNEL_OFFSET_ABOVE) {
                ht40plus = 1;
                ht40minus = 0;
            } else if (p_ap_params->ch_offset == CHANNEL_OFFSET_BELOW) {
                ht40plus = 0;
                ht40minus = 1;
            }
            fprintf(f, "ht_capab=%s%s\n",
                ht40plus ? "[HT40+]" : "",
                ht40minus ? "[HT40-]" : "");
        }
    } else {
        int channel = p_ap_params->channel;
        int ucVhtChannelWidth = 1;
        int ucVhtChannelFrequencyS1 = 0;

        /* 5G */
        fprintf(f, "hw_mode=a\n");
        fprintf(f, "ieee80211ac=1\n");

        if (p_ap_params->ch_width == CHANNEL_WIDTH_20) {
            ucVhtChannelWidth = 0;
            ucVhtChannelFrequencyS1 = 0;
        } else if (p_ap_params->ch_width == CHANNEL_WIDTH_40) {
            ucVhtChannelWidth = 0;
            ucVhtChannelFrequencyS1 = 0;
        } else if (p_ap_params->ch_width == CHANNEL_WIDTH_160) {
            ucVhtChannelWidth = 2;
            if (channel >= 36 && channel <= 64)
                ucVhtChannelFrequencyS1 = 50;
            else if (channel >= 100 && channel <= 128)
                ucVhtChannelFrequencyS1 = 114;
        } else {
            ucVhtChannelWidth = 1;
            if (channel >= 36 && channel <= 48)
                ucVhtChannelFrequencyS1 = 42;
            else if (channel >= 52 && channel <= 64)
                ucVhtChannelFrequencyS1 = 58;
            else if (channel >= 100 && channel <= 112)
                ucVhtChannelFrequencyS1 = 106;
            else if (channel >= 116 && channel <= 128)
                ucVhtChannelFrequencyS1 = 122;
            else if (channel >= 132 && channel <= 144)
                ucVhtChannelFrequencyS1 = 138;
            else if (channel >= 149 && channel <= 161)
                ucVhtChannelFrequencyS1 = 155;
        }

        if (p_ap_params->ch_width != CHANNEL_WIDTH_20) {
            /* Force secondary channel offset to 40+ in 11ax mode */
            ht40plus = 1;
            ht40minus = 0;

            fprintf(f, "ht_capab=%s%s\n",
                ht40plus ? "[HT40+]" : "",
                ht40minus ? "[HT40-]" : "");
        }

        fprintf(f, "vht_oper_centr_freq_seg0_idx=%d\n",ucVhtChannelFrequencyS1);
        fprintf(f, "vht_oper_chwidth=%d\n", ucVhtChannelWidth);
        fprintf(f, "he_oper_centr_freq_seg0_idx=%d\n", ucVhtChannelFrequencyS1);
        fprintf(f, "he_oper_chwidth=%d\n", ucVhtChannelWidth);
    }

    fprintf(f, "he_su_beamformer=0\n");
    fprintf(f, "he_su_beamformee=0\n");
    fprintf(f, "he_mu_beamformer=0\n");
    fprintf(f, "he_twt_required=0\n");

}

static void config_driver_he_params(FILE *f, ap_params_t *p_ap_params)
{
    enum ENUM_CHANNEL_WIDTH ch_width;

    if (f == NULL || p_ap_params == NULL)
        return;

    if (p_ap_params->mode != AP_MODE_11ax &&
        p_ap_params->program != PROGRAME_TYPE_HE)
        return;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    /// Default value
    fprintf(f, "SGCfg 0\n");
    fprintf(f, "SwTestMode 0x16\n");
    fprintf(f, "StaHE 2\n");
    fprintf(f, "StaHEBfee 0\n");

    ch_width = p_ap_params->ch_width;

    if (!ch_width) {
        DPRINT_INFO(WFA_OUT, "ch_width AUTO\n");
        if (p_ap_params->channel <= 14)
            ch_width = CHANNEL_WIDTH_20;
        else
            ch_width = CHANNEL_WIDTH_80;
    }

    if (ch_width == CHANNEL_WIDTH_20)
        fprintf(f, "ApBw 0\n");
    else if (ch_width == CHANNEL_WIDTH_40)
        fprintf(f, "ApBw 1\n");
    else if (ch_width == CHANNEL_WIDTH_80)
        fprintf(f, "ApBw 2\n");
    else if (ch_width == CHANNEL_WIDTH_160)
        fprintf(f, "ApBw 3\n");
    else if (ch_width == CHANNEL_WIDTH_80_80)
        fprintf(f, "ApBw 4\n");

    if (ch_width == CHANNEL_WIDTH_20)
        fprintf(f, "StaBw 0\n");
    else if (ch_width == CHANNEL_WIDTH_40)
        fprintf(f, "StaBw 1\n");
    else if (ch_width == CHANNEL_WIDTH_80)
        fprintf(f, "StaBw 2\n");
    else if (ch_width == CHANNEL_WIDTH_160)
        fprintf(f, "StaBw 3\n");
    else if (ch_width == CHANNEL_WIDTH_80_80)
        fprintf(f, "StaBw 4\n");
}

static void config_he_commit()
{
    if (gDut.ap_params.mode == AP_MODE_11ax ||gDut.program == PROGRAME_TYPE_HE) {
        DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

        if (gDut.ap_params.bcc_mode) {
            DPRINT_INFO(WFA_OUT, "set BCC mode\n");
            shell(IWPRIV" %s driver \"set_chip raCtrl 0x51 0xff00\"",
                gDut.ap_params.inf_name);
        }

        DPRINT_INFO(WFA_OUT, "mcs_fixedrate = %d %d\n",
            gDut.ap_params.mcs_fixedrate,
            gDut.ap_params.mcs_fixedrate_rate);

        if (gDut.ap_params.mcs_fixedrate) {
            shell(IWPRIV" %s driver \"set_chip raCtrl 0x4F 0xff%02x\"",
                gDut.ap_params.inf_name,
                gDut.ap_params.mcs_fixedrate_rate);

            if (gDut.ap_params.mcs_fixedrate_rate == 7)
                shell(IWPRIV" %s driver \"SET_MCS_MAP 0\"",gDut.ap_params.inf_name);
            else if (gDut.ap_params.mcs_fixedrate_rate == 8)
                shell(IWPRIV" %s driver \"SET_MCS_MAP 1\"",gDut.ap_params.inf_name);
            else if (gDut.ap_params.mcs_fixedrate_rate == 9)
                shell(IWPRIV" %s driver \"SET_MCS_MAP 2\"",gDut.ap_params.inf_name);
        } else {
            shell(IWPRIV" %s driver \"SET_MCS_MAP 3\"",gDut.ap_params.inf_name);
        }

    }
}

int wfaApSetRFeature(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;
    caAPRFeat_t *apSetRFeat = (caAPRFeat_t *) caCmdBuf;
    ap_params_t *p_ap_params = &gDut.ap_params;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (apSetRFeat->program)
        p_ap_params->program = apSetRFeat->program;

    if (strlen(apSetRFeat->dutName) > 0)
        wSTRNCPY(p_ap_params->device_name, apSetRFeat->dutName,
                sizeof(p_ap_params->device_name) - 1);

    DPRINT_INFO(WFA_OUT, "\nwfaApSetRFeature program=%d\n", apSetRFeat->program);
    DPRINT_INFO(WFA_OUT, "\nwfaApSetRFeature bw=%d\n", apSetRFeat->width);
    DPRINT_INFO(WFA_OUT, "\nwfaApSetRFeature old_bw=%d\n", p_ap_params->ch_width);

    if (p_ap_params->program == PROGRAME_TYPE_HE) {
        if (apSetRFeat->width) {
            int old_bw = p_ap_params->ch_width;

            p_ap_params->ch_width = apSetRFeat->width;
            if (old_bw != p_ap_params->ch_width)
                wfaApConfigCommit(len, caCmdBuf, respLen, respBuf);
        }

        if (strlen(apSetRFeat->ltf) > 0) {
            DPRINT_INFO(WFA_OUT, "\nwfaApSetRFeature HE LTF=%s\n", apSetRFeat->ltf);
            if (str_same(apSetRFeat->ltf, "3.2"))
                shell(IWPRIV" %s driver \"set_chip raCtrl 0x55 0xff00\"", p_ap_params->inf_name);
            else if (str_same(apSetRFeat->ltf, "6.4"))
                shell(IWPRIV" %s driver \"set_chip raCtrl 0x55 0xff55\"", p_ap_params->inf_name);
            else if (str_same(apSetRFeat->ltf, "12.8"))
                shell(IWPRIV" %s driver \"set_chip raCtrl 0x55 0xffaa\"", p_ap_params->inf_name);
        }
        if (strlen(apSetRFeat->gi) > 0) {
            DPRINT_INFO(WFA_OUT, "\nwfaApSetRFeature HE GI=%s\n", apSetRFeat->gi);
            if (str_same(apSetRFeat->gi, "0.8"))
                shell(IWPRIV" %s driver \"set_chip raCtrl 0x54 0xff00\"", p_ap_params->inf_name);
            else if (str_same(apSetRFeat->gi, "1.6"))
                shell(IWPRIV" %s driver \"set_chip raCtrl 0x54 0xff55\"", p_ap_params->inf_name);
            else if (str_same(apSetRFeat->gi, "3.2"))
                shell(IWPRIV" %s driver \"set_chip raCtrl 0x54 0xffaa\"", p_ap_params->inf_name);
        }
    }

    resp->status = STATUS_COMPLETE;

    wfaEncodeTLV(WFA_AP_SET_RFEATURE_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaApGetVersion(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *getVerResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    getVerResp->status = STATUS_COMPLETE;
    wSTRNCPY(getVerResp->cmdru.version, MTK_SYSTEM_VER, sizeof(getVerResp->cmdru.version));
    getVerResp->cmdru.version[sizeof(getVerResp->cmdru.version) - 1] = '\0';

    wfaEncodeTLV(WFA_AP_CA_VERSION_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)getVerResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaApConfigCommit(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;
    char command[1000];

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    write_hostapd_conf();
    write_driver_conf();

    if (shell("killall hostapd") != WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "killall hostapd fail\n");
    if (shell("echo 0 > /dev/wmtWifi") != WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "turn off wifi driver fail\n");
    wSLEEP(1);

    if (shell("echo 1 > /dev/wmtWifi") != WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "turn on wifi driver fail\n");
    if (shell("echo AP > /dev/wmtWifi") != WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "change wifi driver to AP mode fail\n");
    wMEMSET(&command, 0, sizeof(command));
    wSNPRINTF(command, sizeof(command), "%s -ddd %s &",
            gDut.ap_params.hostapd_bin,
            gDut.ap_params.hostapd_conf);
    if (system(command) != 0)
        DPRINT_WARNING(WFA_WNG, "turn on hostapd fail\n");

    setup_ap_bridge_n_address();

    wUSLEEP(500000);

    if (run_hostapd_cli("ping") == WFA_SUCCESS)
        DPRINT_INFO(WFA_OUT, "ping resp: %s\n", gDut.shellResult);

    wSLEEP(5);

    if (gDut.ap_params.mode == AP_MODE_11ax || gDut.ap_params.program == PROGRAME_TYPE_HE) {
        config_he_commit();
    } else {
        /* notify FW sigma test mode to disable fast-tx */
        if (gDut.ap_params.mode == AP_MODE_11ac) {
            if (shell(IWPRIV " %s set_sw_ctrl 0xa0400000 0xD",
                gDut.ap_params.inf_name) != WFA_SUCCESS)
                DPRINT_WARNING(WFA_WNG, "set_sw_ctrl 0xD fail\n");
        } else {
            if (shell(IWPRIV " %s set_sw_ctrl 0xa0400000 0xE",
                gDut.ap_params.inf_name) != WFA_SUCCESS)
                DPRINT_WARNING(WFA_WNG, "set_sw_ctrl 0xE fail\n");
        }
    }

    resp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_AP_CONFIG_COMMIT_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaApDeauthSta(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;
    dutCommand_t *deauthSta = (dutCommand_t *) caCmdBuf;
    char buf[100];

    DPRINT_INFO(WFA_OUT, "Entering %s(), sta mac: %s\n", __func__,
            deauthSta->cmdsu.macaddr);

    wSNPRINTF(buf, sizeof(buf), "deauth %s", deauthSta->cmdsu.macaddr);
    run_hostapd_cli(buf);

    resp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_AP_DEAUTH_STA_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaApGetMacAddress(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    unsigned char addr[6];
    dutCmdResponse_t *getMacResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    getMacResp->status = get_mac(gDut.ap_params.inf_name, addr);
    wSNPRINTF(getMacResp->cmdru.mac, sizeof(getMacResp->cmdru.mac),
            "%02x:%02x:%02x:%02x:%02x:%02x",
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    DPRINT_INFO(WFA_OUT, "mac=%s\n", getMacResp->cmdru.mac);

    wfaEncodeTLV(WFA_AP_GET_MAC_ADDRESS_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)getMacResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
    return WFA_SUCCESS;
}

int wfaApResetDefault(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;
    caApResetDefault_t *apResetDefault_t = (caApResetDefault_t *) caCmdBuf;

    DPRINT_INFO(WFA_OUT, "Entering %s(), program: %d\n", __func__,
            apResetDefault_t->program);

    gDut.ap_params.program = apResetDefault_t->program;

    wMEMSET(&gDut.ap_params, 0, sizeof(ap_params_t));

    init_ap_params();

    if (shell("killall hostapd") != WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "killall hostapd fail\n");
    if (shell("echo 0 > /dev/wmtWifi") != WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "turn off wifi driver fail\n");
    wSLEEP(1);

    resp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_AP_RESET_DEFAULT_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaApSendAddBaReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "Entering %s(), Not support now, skip.\n", __func__);

    resp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_AP_SEND_ADDBA_REQ_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaApSetApQos(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "Entering %s(), Not support now, skip.\n", __func__);

    resp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_AP_SET_APQOS_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaApSetPmf(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    /*
     * Ignore this command since the parameters are already
     * handled by xcCmdProcApSetSecurity */

    resp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_AP_SET_PMF_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaApSetSecurity(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;
    caApSetSecurity_t *apSetSecurity = (caApSetSecurity_t *) caCmdBuf;
    ap_params_t *p_ap_params = &gDut.ap_params;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (apSetSecurity->keyMgmtType)
        p_ap_params->keyMgmtType = apSetSecurity->keyMgmtType;

    if (apSetSecurity->ap_wepkey[0])
        wSTRNCPY(p_ap_params->wepkey, apSetSecurity->ap_wepkey,
                sizeof(p_ap_params->wepkey) - 1);

    if (apSetSecurity->encpType)
        p_ap_params->encpType = apSetSecurity->encpType;

    if (apSetSecurity->ap_passphrase[0])
        wSTRNCPY(p_ap_params->passphrase, apSetSecurity->ap_passphrase,
                sizeof(p_ap_params->passphrase) - 1);

    if (apSetSecurity->pmf)
        p_ap_params->pmf = apSetSecurity->pmf;

    if (apSetSecurity->sha256ad)
        p_ap_params->sha256ad = apSetSecurity->sha256ad;

    if (apSetSecurity->preauthentication)
        p_ap_params->preauthentication = apSetSecurity->preauthentication;

    if (apSetSecurity->antiCloggingThreshold)
        p_ap_params->antiCloggingThreshold = apSetSecurity->antiCloggingThreshold;

    if (apSetSecurity->ap_reflection)
        p_ap_params->ap_reflection = apSetSecurity->ap_reflection;

    if (apSetSecurity->ap_ecGroupID[0])
        wSTRNCPY(p_ap_params->ap_ecGroupID, apSetSecurity->ap_ecGroupID,
                sizeof(p_ap_params->ap_ecGroupID) - 1);

    if (apSetSecurity->ap_sae_commit_override[0])
        wSTRNCPY(p_ap_params->ap_sae_commit_override, apSetSecurity->ap_sae_commit_override,
                sizeof(p_ap_params->ap_sae_commit_override) - 1);

    resp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_AP_SET_SECURITY_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaApSetStaQos(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "Entering %s(), Not support now, skip.\n", __func__);

    resp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_AP_SET_STAQOS_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaApSetWireless(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;
    caApSetWireless_t *apSetWireless = (caApSetWireless_t *) caCmdBuf;
    ap_params_t *p_ap_params = &gDut.ap_params;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (apSetWireless->program)
        p_ap_params->program = apSetWireless->program;

    if (strlen(apSetWireless->ssid) > 0)
        wSTRNCPY(p_ap_params->ssid, apSetWireless->ssid,
                sizeof(p_ap_params->ssid) - 1);

    if (strlen(apSetWireless->dutName) > 0)
        wSTRNCPY(p_ap_params->device_name, apSetWireless->dutName,
                sizeof(p_ap_params->device_name) - 1);

    if (apSetWireless->channel)
        p_ap_params->channel = apSetWireless->channel;

    if (apSetWireless->rts)
        p_ap_params->rts = apSetWireless->rts;

    if (apSetWireless->fragment)
        p_ap_params->frgmnt = apSetWireless->fragment;

    if (apSetWireless->beaconInterval)
        p_ap_params->bcnint = apSetWireless->beaconInterval;

    if (apSetWireless->dtim)
        p_ap_params->dtim_period = apSetWireless->dtim;

    if (apSetWireless->wme)
        p_ap_params->wme = apSetWireless->wme;

    if (apSetWireless->wmmps)
        p_ap_params->wmmps = apSetWireless->wmmps;

    if (apSetWireless->p2p_mgmt)
        p_ap_params->p2p_mgmt = apSetWireless->p2p_mgmt;

    if (apSetWireless->mode)
        p_ap_params->mode = apSetWireless->mode;

    if (apSetWireless->offset)
        p_ap_params->ch_offset = apSetWireless->offset;

    if (apSetWireless->sgi20)
        p_ap_params->sgi20 = apSetWireless->sgi20;

    if (apSetWireless->program == PROGRAME_TYPE_HE) {
        DPRINT_INFO(WFA_OUT, "width no_update: %d\n", apSetWireless->width);

        if (apSetWireless->bcc_mode)
            p_ap_params->bcc_mode = apSetWireless->bcc_mode;

        if (apSetWireless->mcs_fixedrate) {
            p_ap_params->mcs_fixedrate = apSetWireless->mcs_fixedrate;
            p_ap_params->mcs_fixedrate_rate = apSetWireless->mcs_fixedrate_rate;
        }

        if (apSetWireless->width)
            p_ap_params->ch_width = apSetWireless->width;
    }

    resp->status = STATUS_COMPLETE;

    wfaEncodeTLV(WFA_AP_SET_WIRELESS_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaApAccessPoint(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *resp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "Entering %s(), do nothing.\n", __func__);

    resp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_AP_ACCESS_POINT_RESP_TLV, sizeof(dutCmdResponse_t),
            (BYTE *)resp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

static char *getUsbInterface()
{
    if (is_interface_up(USB0_INF_NAME))
        return USB0_INF_NAME;

    return USB1_INF_NAME;
}

static void init_ap_params()
{
    int sdk;
    char propertyValue[128] = {'\0'};

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    gDut.ap_params.keyMgmtType = KEY_MGNT_TYPE_OPEN;

    wSNPRINTF(gDut.ap_params.inf_name, sizeof(gDut.ap_params.inf_name), "%s",
            "ap0");
    wSNPRINTF(gDut.ap_params.hostapd_conf, sizeof(gDut.ap_params.hostapd_conf),
            "%s", "/data/local/sigma_hostapd.conf");
    wSNPRINTF(gDut.ap_params.driver_conf, sizeof(gDut.ap_params.driver_conf),
            "%s", "/data/misc/wifi/wifi.cfg");

    wMEMSET(&propertyValue, 0, sizeof(propertyValue));
    if (shell("getprop vendor.wifi.sigma.ip.addr") == WFA_SUCCESS)
        wSTRNCPY(propertyValue, gDut.shellResult, sizeof(propertyValue) - 1);
    wSNPRINTF(gDut.ap_params.ip_addr, sizeof(gDut.ap_params.ip_addr), "%s",
            propertyValue);

    wMEMSET(&propertyValue, 0, sizeof(propertyValue));
    if (shell("getprop vendor.wifi.sigma.ip.netmask") == WFA_SUCCESS)
        wSTRNCPY(propertyValue, gDut.shellResult, sizeof(propertyValue) - 1);
    wSNPRINTF(gDut.ap_params.ip_netmask, sizeof(gDut.ap_params.ip_netmask), "%s",
            propertyValue);

    wMEMSET(&propertyValue, 0, sizeof(propertyValue));
    if (shell("getprop ro.build.version.sdk") == WFA_SUCCESS)
        wSTRNCPY(propertyValue, gDut.shellResult, sizeof(propertyValue) - 1);
    sdk = atoi(propertyValue);
    if (sdk >= 19 && sdk < 26) { // KK ~ N
        wSNPRINTF(gDut.ap_params.ctrl_inf, sizeof(gDut.ap_params.ctrl_inf), "%s",
                "data/misc/wifi/hostapd");
        wSNPRINTF(gDut.ap_params.hostapd_bin, sizeof(gDut.ap_params.hostapd_bin),
                "%s", "/system/bin/hostapd");
    } else if (sdk >= 26 && sdk < 28) { // O
        wSNPRINTF(gDut.ap_params.ctrl_inf, sizeof(gDut.ap_params.ctrl_inf), "%s",
                "data/misc/wifi/hostapd/ctrl");
        wSNPRINTF(gDut.ap_params.hostapd_bin, sizeof(gDut.ap_params.hostapd_bin),
                "%s", "/vendor/bin/hostapd");
    } else { // P ~
        wSNPRINTF(gDut.ap_params.ctrl_inf, sizeof(gDut.ap_params.ctrl_inf), "%s",
                "data/vendor/wifi/hostapd/ctrl");
        wSNPRINTF(gDut.ap_params.hostapd_bin, sizeof(gDut.ap_params.hostapd_bin),
                "%s", "/vendor/bin/hw/hostapd");
    }
}

static void config_hostapd_htcap(FILE *f, ap_params_t *p_ap_params)
{
    int ht40plus = 0, ht40minus = 0;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (f == NULL || p_ap_params == NULL) {
        DPRINT_WARNING(WFA_WNG, "invalid args\n");
        return;
    }

    if (p_ap_params->mode != AP_MODE_11ng &&
            p_ap_params->mode != AP_MODE_11na &&
            p_ap_params->mode != AP_MODE_11ac)
        return;

    if (p_ap_params->ch_width == CHANNEL_WIDTH_40 &&
            p_ap_params->ch_offset == CHANNEL_OFFSET_ABOVE) {
        ht40plus = 1;
        ht40minus = 0;
    } else if (p_ap_params->ch_width == CHANNEL_WIDTH_40 &&
            p_ap_params->ch_offset == CHANNEL_OFFSET_BELOW) {
        ht40plus = 0;
        ht40minus = 1;
    } else if (p_ap_params->mode == AP_MODE_11ac) {
        /* Force secondary channel offset to 40+ in 11ac mode */
        ht40plus = 1;
        ht40minus = 0;
    } else {
        ht40plus = 0;
        ht40minus = 0;
        DPRINT_INFO(WFA_OUT, "no channel offset setting.\n");
    }
    if (!ht40plus && !ht40minus && p_ap_params->sgi20 != WFA_ENABLED)
        return;
    fprintf(f, "ht_capab=%s%s%s\n",
        ht40plus ? "[HT40+]" : "",
        ht40minus ? "[HT40-]" : "",
        p_ap_params->sgi20 == WFA_ENABLED ? "[SHORT-GI-20]" : "");
}

static void config_driver_wmm_params(FILE *f, ap_params_t *p_ap_params)
{
    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (f == NULL || p_ap_params == NULL)
        return;

    if (p_ap_params->mode == AP_MODE_11ng) {
        switch (gDut.driver_gen) {
        case GEN_2:
            fprintf(f, "BeAifsN 3");
            fprintf(f, "BeCwMin 3");
            fprintf(f, "BeCwMax 6");
            fprintf(f, "BkAifsN 8");
            fprintf(f, "BkCwMin 4");
            fprintf(f, "BkCwMax 10");
            break;
        case GEN_3:
            fprintf(f, "BeAifsN 3");
            fprintf(f, "BeCwMin 8");
            fprintf(f, "BeCwMax 63");
            fprintf(f, "BkAifsN 8");
            fprintf(f, "BkCwMin 15");
            fprintf(f, "BkCwMax 1024");
            break;
        default:
            break;
        }
    }
}

static void write_hostapd_conf()
{
    FILE *f;
    ap_params_t *p_ap_params = &gDut.ap_params;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    f = fopen("/data/local/sigma_hostapd.conf", "w");
    if (f == NULL) {
        DPRINT_ERR(WFA_ERR, "file open fail\n");
        goto exit;
    }

    fprintf(f, "interface=%s\n", p_ap_params->inf_name);
    fprintf(f, "ctrl_interface=%s\n", p_ap_params->ctrl_inf);

    if (strlen(p_ap_params->ssid) > 0)
        fprintf(f, "ssid=%s\n", p_ap_params->ssid);

    if (p_ap_params->device_name[0])
        fprintf(f, "device_name=%s\n", p_ap_params->device_name);

    switch (p_ap_params->mode) {
    case AP_MODE_11b:
        fprintf(f, "hw_mode=b\n");
        break;
    case AP_MODE_11g:
    case AP_MODE_11ng:
        fprintf(f, "hw_mode=g\n");
        if (p_ap_params->mode == AP_MODE_11ng) {
            fprintf(f, "ieee80211n=1\n");
        }
        break;
    case AP_MODE_11a:
    case AP_MODE_11na:
    case AP_MODE_11ac:
        fprintf(f, "hw_mode=a\n");
        if (p_ap_params->mode == AP_MODE_11na ||
                p_ap_params->mode == AP_MODE_11ac) {
            fprintf(f, "ieee80211n=1\n");
        }
        if (p_ap_params->mode == AP_MODE_11ac) {
            fprintf(f, "ieee80211ac=1\n");
            if (p_ap_params->country_code[0])
                fprintf(f, "country_code=%s\n", p_ap_params->country_code);
        }
        break;
    case AP_MODE_11ax:
        config_hostapd_he(f, p_ap_params);
        break;
    default:
        DPRINT_WARNING(WFA_WNG, "unsupport mode: %d\n", p_ap_params->mode);
        goto exit;
    }
    config_hostapd_htcap(f, p_ap_params);
    if (p_ap_params->channel)
        fprintf(f, "channel=%d\n", p_ap_params->channel);
    if (p_ap_params->bcnint)
        fprintf(f, "beacon_int=%d\n", p_ap_params->bcnint);
    if (p_ap_params->rts)
        fprintf(f, "rts_threshold=%d\n", p_ap_params->rts);
    if (p_ap_params->frgmnt)
        fprintf(f, "fragm_threshold=%d\n", p_ap_params->frgmnt);
    if (p_ap_params->dtim_period)
        fprintf(f, "dtim_period=%d\n", p_ap_params->dtim_period);
    if (p_ap_params->wme)
        fprintf(f, "wmm_enabled=1\n");
    if (p_ap_params->wmmps)
        fprintf(f, "uapsd_advertisement_enabled=1\n");
    if (p_ap_params->p2p_mgmt)
        fprintf(f, "manage_p2p=1\n");

    switch (p_ap_params->keyMgmtType) {
    case KEY_MGNT_TYPE_OPEN:
        if (p_ap_params->wepkey[0])
            fprintf(f, "wep_key0=%s\n", p_ap_params->wepkey);
        break;
    case KEY_MGNT_TYPE_WPA_PSK:
    case KEY_MGNT_TYPE_WPA2_PSK:
    case KEY_MGNT_TYPE_WPA2_PSK_MIXED:
        /* config wpa */
        if (p_ap_params->keyMgmtType == KEY_MGNT_TYPE_WPA2_PSK) {
            fprintf(f, "rsn_pairwise=CCMP\n");
            fprintf(f, "wpa=2\n");
        } else if (p_ap_params->keyMgmtType == KEY_MGNT_TYPE_WPA2_PSK_MIXED) {
            fprintf(f, "wpa=3\n");
        } else {
            fprintf(f, "wpa=1\n");
        }

        switch (p_ap_params->encpType) {
        case ENCP_TYPE_NONE:
            /* do nothing*/
            break;
        case ENCP_TYPE_TKIP:
            fprintf(f, "wpa_pairwise=TKIP\n");
            break;
        case ENCP_TYPE_CCMP:
            fprintf(f, "wpa_pairwise=CCMP\n");
            break;
        case ENCP_TYPE_GCMP_128:
            fprintf(f, "wpa_pairwise=GCMP\n");
            break;
        case ENCP_TYPE_CCMP_TKIP:
            fprintf(f, "wpa_pairwise=CCMP TKIP\n");
            break;
        default:
            DPRINT_WARNING(WFA_WNG, "unknown encpType: %d",
                    p_ap_params->encpType);
            break;
        }

        if (p_ap_params->passphrase[0])
            fprintf(f, "wpa_passphrase=%s\n", p_ap_params->passphrase);

        switch (p_ap_params->pmf) {
        case WFA_DISABLED:
        case WFA_OPTIONAL:
            if (p_ap_params->sha256ad)
                fprintf(f, "wpa_key_mgmt=WPA-PSK WPA-PSK-SHA256\n");
            else
                fprintf(f, "wpa_key_mgmt=WPA-PSK\n");
            break;
        case WFA_REQUIRED:
            fprintf(f, "WPA-PSK-SHA256\n");
            break;
        default:
            break;
        }
        break;

    case KEY_MGNT_TYPE_WPA2_PSK_SAE:
    case KEY_MGNT_TYPE_WPA2_SAE:
    {
        const char *key_mgmt;

        fprintf(f, "wpa=2\n");

        switch (p_ap_params->pmf) {
        case WFA_DISABLED:
        case WFA_OPTIONAL:
            if (p_ap_params->keyMgmtType == KEY_MGNT_TYPE_WPA2_SAE) {
                key_mgmt = "SAE";
            } else {
                key_mgmt = "WPA-PSK SAE";
            }
            fprintf(f, "wpa_key_mgmt=%s%s\n", key_mgmt, p_ap_params->sha256ad ? " WPA-PSK-SHA256" : "");
            break;
        case WFA_REQUIRED:
            if (p_ap_params->keyMgmtType == KEY_MGNT_TYPE_WPA2_SAE) {
                key_mgmt = "SAE";
            } else {
                key_mgmt = "WPA-PSK-SHA256 SAE";
            }
            fprintf(f, "wpa_key_mgmt=%s\n", key_mgmt);
            break;
        default:
            break;
        }

        switch (p_ap_params->encpType) {
        case ENCP_TYPE_NONE:
            /* do nothing*/
            break;
        case ENCP_TYPE_TKIP:
            fprintf(f, "wpa_pairwise=TKIP\n");
            break;
        case ENCP_TYPE_CCMP:
            fprintf(f, "wpa_pairwise=CCMP\n");
            break;
        case ENCP_TYPE_GCMP_128:
            fprintf(f, "wpa_pairwise=GCMP\n");
            break;
        case ENCP_TYPE_CCMP_TKIP:
            fprintf(f, "wpa_pairwise=CCMP TKIP\n");
            break;
        default:
            DPRINT_WARNING(WFA_WNG, "unknown encpType: %d",
                    p_ap_params->encpType);
            break;
        }

        if (p_ap_params->keyMgmtType == KEY_MGNT_TYPE_WPA2_SAE) {
            ///fprintf(f, "sae_require_mfp=2\n");
            if (p_ap_params->passphrase[0])
                fprintf(f, "sae_password=%s\n", p_ap_params->passphrase);
        } else {
            if (p_ap_params->passphrase[0]) {
                fprintf(f, "wpa_passphrase=%s\n", p_ap_params->passphrase);
                fprintf(f, "sae_password=%s\n", p_ap_params->passphrase);
            }
        }
        break;
    }
    case KEY_MGNT_TYPE_WPA_EAP:
    case KEY_MGNT_TYPE_WPA2_EAP:
    case KEY_MGNT_TYPE_WPA2_EAP_MIXED:
    case KEY_MGNT_TYPE_SUITEB:
        DPRINT_WARNING(WFA_WNG, "not support keyMgmtType: %d",
                p_ap_params->keyMgmtType);
        break;
    default:
        DPRINT_WARNING(WFA_WNG, "unknown keyMgmtType: %d",
                p_ap_params->keyMgmtType);
        break;
    }

    switch (p_ap_params->pmf) {
    case WFA_DISABLED:
        /* do nothing */
        break;
    case WFA_OPTIONAL:
        fprintf(f, "ieee80211w=1\n");
        if (p_ap_params->keyMgmtType == KEY_MGNT_TYPE_WPA2_PSK_SAE) {
            fprintf(f, "sae_require_mfp=1\n");
        }
        break;
    case WFA_REQUIRED:
        fprintf(f, "ieee80211w=2\n");
        break;
    default:
        break;
    }

    if (p_ap_params->preauthentication)
        fprintf(f, "rsn_preauth=1\n");

    if (p_ap_params->ap_ecGroupID[0])
        fprintf(f, "sae_groups=%s\n", p_ap_params->ap_ecGroupID);

    if (p_ap_params->ap_sae_commit_override[0])
        fprintf(f, "sae_commit_override=%s\n", p_ap_params->ap_sae_commit_override);

    if (p_ap_params->antiCloggingThreshold >= 0)
        fprintf(f, "sae_anti_clogging_threshold=%d\n", p_ap_params->antiCloggingThreshold);

    if (p_ap_params->ap_reflection)
        fprintf(f, "sae_reflection_attack=1\n");

exit:
    if (f != NULL)
        fclose(f);
    if (shell("chmod 0777 /data/local/sigma_hostapd.conf") != WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "chmod for hostapd.conf fail\n");
    if (shell("chown wifi:system /data/local/sigma_hostapd.conf") !=
            WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "chown for hostapd.conf fail\n");
}

static void write_driver_conf()
{
    FILE *f;
    ap_params_t *p_ap_params = &gDut.ap_params;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    f = fopen(gDut.ap_params.driver_conf, "w");
    if (f == NULL) {
        DPRINT_ERR(WFA_ERR, "file open fail\n");
        goto exit;
    }
    if (p_ap_params->wmmps && p_ap_params->wme)
        fprintf(f, "ApUapsd 1\n");
    if (p_ap_params->sig_rts)
        fprintf(f, "SigTaRts 1\n");
    if (p_ap_params->dynamic_bw_signaling)
        fprintf(f, "DynBwRts 1\n");
    /* Disable 2G Vht and Probe256QAM to fix 4.2.43_GN bug */
    fprintf(f, "VhtIeIn2G 0\n");
    fprintf(f, "Probe256QAM 0\n");
    config_driver_wmm_params(f, p_ap_params);
    config_driver_he_params(f, p_ap_params);
exit:
    if (f != NULL)
        fclose(f);
    if (shell("chmod 0777 %s", gDut.ap_params.driver_conf) != WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "chmod for driver config fail\n");
}

static int run_hostapd_cli(char *buf)
{
    DPRINT_INFO(WFA_OUT, "Entering %s(), buf: %s\n", __func__, buf);

    return shell("hostapd_cli -i%s -p %s %s",
            gDut.ap_params.inf_name,
            gDut.ap_params.ctrl_inf,
            buf);
}

static int is_interface_up(char *inf)
{
    struct ifreq ifr;
    int sock = wSOCKET(PF_INET6, SOCK_DGRAM, IPPROTO_IP);

    if (sock < 0)
        return 0;

    wMEMSET(&ifr, 0, sizeof(ifr));
    wSTRNCPY(ifr.ifr_name, inf, sizeof(ifr.ifr_name) - 1);
    if (wIOCTL(sock, SIOCGIFFLAGS, &ifr) < 0) {
        DPRINT_WARNING(WFA_WNG, "Get interface flags fail: %s\n",
                AP_BRIDGE_INF_NAME);
        wCLOSE(sock);
        return 0;
    }
    wCLOSE(sock);
    return !!(ifr.ifr_flags & IFF_UP);
}

static void mtk_del_ap_bridge()
{
    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (shell("ifconfig " AP_BRIDGE_INF_NAME " down") != WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "Set %s interface down fail\n",
                AP_BRIDGE_INF_NAME);
    mtk_bridge_intf_setup(AP_BRIDGE_INF_NAME, gDut.ap_params.inf_name,
            BRIDGE_OP_TYPE_DEL_INTF);
    mtk_bridge_intf_setup(AP_BRIDGE_INF_NAME, getUsbInterface(),
            BRIDGE_OP_TYPE_DEL_INTF);
    mtk_bridge_setup(AP_BRIDGE_INF_NAME, BRIDGE_OP_TYPE_DEL_BR);
}

static void mtk_add_ap_bridge()
{
    int ret;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (is_interface_up(AP_BRIDGE_INF_NAME)) {
        DPRINT_WARNING(WFA_WNG, "Interface %s already exists.\n",
                AP_BRIDGE_INF_NAME);
        return;
    }

    /* Clear rndis0's ip address */
    if (shell("ifconfig %s 0.0.0.0",
                getUsbInterface()) != WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "Clear interface %s's address fail\n",
                getUsbInterface());

    /* Create bridge br0 */
    ret = mtk_bridge_setup(AP_BRIDGE_INF_NAME, BRIDGE_OP_TYPE_ADD_BR);

    /* Bind br0 with ap0 & rndis0 */
    ret = mtk_bridge_intf_setup(AP_BRIDGE_INF_NAME, gDut.ap_params.inf_name,
            BRIDGE_OP_TYPE_ADD_INTF);
    ret = mtk_bridge_intf_setup(AP_BRIDGE_INF_NAME, getUsbInterface(),
            BRIDGE_OP_TYPE_ADD_INTF);

    /* Set br0's ip & netmask' */
    if (shell("ifconfig %s %s netmask %s up",
            AP_BRIDGE_INF_NAME,
            gDut.ap_params.ip_addr,
            gDut.ap_params.ip_netmask) != WFA_SUCCESS)
        DPRINT_WARNING(WFA_WNG, "Setup %s ip/netmask fail\n",
                AP_BRIDGE_INF_NAME);
}

static void setup_ap_bridge_n_address()
{
    mtk_del_ap_bridge();

    if (!is_interface_up(getUsbInterface())) {
        DPRINT_INFO(WFA_OUT, "Setup %s's address directly since %s not existed.\n",
                gDut.ap_params.inf_name, getUsbInterface());
        if (shell("ifconfig %s %s netmask %s up",
                gDut.ap_params.inf_name,
                gDut.ap_params.ip_addr,
                gDut.ap_params.ip_netmask) != WFA_SUCCESS)
            DPRINT_WARNING(WFA_WNG, "Setup %s ip/netmask fail\n",
                    gDut.ap_params.inf_name);
        return;
    }

    mtk_add_ap_bridge();

    /* Disable iptables on br0 */
    shell("echo 0 > /proc/sys/net/bridge/bridge-nf-call-ip6tables");
    shell("echo 0 > /proc/sys/net/bridge/bridge-nf-call-iptables");
    shell("echo 0 > /proc/sys/net/bridge/bridge-nf-call-arptables");
}

static int mtk_bridge_setup(const char *brname, enum ENUM_BRIDGE_OP_TYPE type)
{
    int ret = 0;
    int br_ctrl_fd = -1;

    if (type != BRIDGE_OP_TYPE_ADD_BR && type != BRIDGE_OP_TYPE_DEL_BR) {
        ret = -EINVAL;
        goto exit;
    }

    br_ctrl_fd = wSOCKET(AF_LOCAL, SOCK_STREAM, 0);
    if (br_ctrl_fd < 0) {
        ret = -ENOENT;
        goto exit;
    }

    ret = wIOCTL(br_ctrl_fd, type == BRIDGE_OP_TYPE_ADD_BR ? SIOCBRADDBR : SIOCBRDELBR, brname);
    if (ret < 0) {
        char _br[IFNAMSIZ];
        unsigned long arg[3] = { type == BRIDGE_OP_TYPE_ADD_BR ? BRCTL_ADD_BRIDGE : BRCTL_DEL_BRIDGE,
                (unsigned long) _br };

        wSTRNCPY(_br, brname, IFNAMSIZ - 1);
        ret = wIOCTL(br_ctrl_fd, SIOCSIFBR, arg);
    }

    if (ret < 0)
        ret = errno;

exit:
    DPRINT_INFO(WFA_ERR, "%s(), brname: %s, type: %d, br_ctrl_fd: %d, ret: %d (%s)\n",
            __func__, brname, type, br_ctrl_fd, ret, ret == 0 ? "NULL" : strerror(errno));
    if (br_ctrl_fd >= 0)
        wCLOSE(br_ctrl_fd);

    return ret;
}

static int mtk_bridge_intf_setup(const char *bridge, const char *dev,
        enum ENUM_BRIDGE_OP_TYPE type)
{
    struct ifreq ifr;
    int ret = 0;
    int ifindex = if_nametoindex(dev);
    int br_ctrl_fd = -1;

    if (type != BRIDGE_OP_TYPE_ADD_INTF && type != BRIDGE_OP_TYPE_DEL_INTF) {
        ret = -EINVAL;
        goto exit;
    }

    br_ctrl_fd = wSOCKET(AF_LOCAL, SOCK_STREAM, 0);
    if (br_ctrl_fd < 0) {
        ret = -ENOENT;
        goto exit;
    }

    if (ifindex == 0) {
        ret = -ENODEV;
        goto exit;
    }

    wSTRNCPY(ifr.ifr_name, bridge, IFNAMSIZ - 1);
    ifr.ifr_ifindex = ifindex;
    ret = wIOCTL(br_ctrl_fd, type == BRIDGE_OP_TYPE_ADD_INTF ? SIOCBRADDIF : SIOCBRDELIF, &ifr);
    if (ret < 0) {
        unsigned long args[4] = { type == BRIDGE_OP_TYPE_ADD_INTF ? BRCTL_ADD_IF : BRCTL_DEL_IF,
                ifindex, 0, 0 };

        ifr.ifr_data = (char *) args;
        ret = wIOCTL(br_ctrl_fd, SIOCDEVPRIVATE, &ifr);
    }

    if (ret < 0)
        ret = errno;

exit:
    DPRINT_INFO(WFA_ERR, "%s(), bridge: %s, dev: %s, type: %d, br_ctrl_fd: %d, ret: %d (%s)\n",
            __func__, bridge, dev, type, br_ctrl_fd, ret, ret == 0 ? "NULL" : strerror(errno));
    if (br_ctrl_fd >= 0)
        wCLOSE(br_ctrl_fd);

    return ret;
}

char *getApMainInterface()
{
    if (!is_interface_up(getUsbInterface())) {
        return gDut.ap_params.inf_name;
    } else {
        return AP_BRIDGE_INF_NAME;
    }
}
#endif

static char *get_main_intf()
{
#ifdef CONFIG_MTK_P2P
    char buf[4096];

    if (wpa_command_resp(WFA_STAUT_IF, "INTERFACES", buf, sizeof(buf)) < 0) {
        DPRINT_WARNING(WFA_WNG, "wpa cmd fail, get interface\n");
        return "";
    }
    if (strstr(buf, "p2p0") && (sigma_mode == SIGMA_MODE_P2P
#ifdef CONFIG_MTK_WFD
        || sigma_mode == SIGMA_MODE_WFD
#endif
        ))
        return "p2p0";
    else
#endif
        return WFA_STAUT_IF;
}

static char *get_sta_intf(const char* ifname) {
    if (sigma_mode == SIGMA_MODE_P2P
#ifdef CONFIG_MTK_WFD
        || sigma_mode == SIGMA_MODE_WFD
#endif
    )
        return WFA_STAUT_IF;
    else
        return (char *)ifname;
}

#ifdef CONFIG_MTK_P2P
static void init_p2p_params()
{
    static pthread_t event_thread;
    char *ifname = "p2p0"; // TODO: hardcode p2p0 interface name

    if (strcmp(get_main_intf(), ifname) != 0)
        return;

    pthread_create(&event_thread, NULL, &p2p_event_receive, ifname);
}

static int p2p_is_peer_known(const char *ifname, const char *peer, int discovered)
{
    char buf[4096];

    wSNPRINTF(buf, sizeof(buf), "P2P_PEER %s", peer);
    if (wpa_command_resp(ifname, buf, buf, sizeof(buf)) < 0) {
        DPRINT_WARNING(WFA_WNG, "wpa cmd fail, %s\n", buf);
        return 0;
    }
    DPRINT_INFO(WFA_OUT, "p2p_is_peer_known buf: %s\n", buf);
    if (strncasecmp(buf, peer, strlen(peer)) != 0)
        return 0;
    if (!discovered)
        return 1;
    return strstr(buf, "[PROBE_REQ_ONLY]") == NULL ? 1 : 0;
}

/**
 * p2p_find_peer - Find p2p peer
 * @ifname: interface name for p2p control interface
 * @peer: peer's mac address
 * @discovered: Fully discovered, i.e. which we have only seen in a received
 *              Probe Request frame.
 * Returns: 1 if peer is found, 0 if peer can NOT be found, or -1 on failure
 */
static int p2p_find_peer(const char *ifname, const char *peer, int discovered)
{
    int count = 0;

    DPRINT_INFO(WFA_OUT, "ifname: %s, peer: %s, discovered: %d\n",
            ifname, peer, discovered);

    if (p2p_is_peer_known(ifname, peer, discovered))
        return 1;

    if (wpa_command(ifname, "P2P_FIND type=progressive") < 0) {
        DPRINT_WARNING(WFA_WNG, "wpa cmd fail, P2P_FIND type=progressive\n");
        return -1;
    }

    while (count < 120) {
        count++;
        wSLEEP(1);
        if (p2p_is_peer_known(ifname, peer, discovered)) {
            DPRINT_INFO(WFA_OUT, "restore to previous state\n");
            switch (gDut.p2p_params.p2p_mode) {
            case P2P_MODE_IDLE:
                wpa_command(ifname, "P2P_STOP_FIND");
                break;
            case P2P_MODE_LISTEN:
                wpa_command(ifname, "P2P_LISTEN");
                break;
            }
            return 1;
        }
    }
    DPRINT_WARNING(WFA_WNG, "p2p find timeout, can not find %s\n", peer);
    return 0;
}

static void enable_dhcp(char *ifname, int enable, int go)
{
    DPRINT_INFO(WFA_OUT, "Entering %s(), ifname: %s, enable: %d, go: %d\n",
            __func__, ifname, enable, go);

    if (enable) {
        if (go) {
            if (shell(DHCPSERVER" %s", ifname) != WFA_SUCCESS)
                DPRINT_WARNING(WFA_WNG, "start p2p dhcp server fail\n");
        } else {
            if (shell(DHCPCLIENT" %s", ifname) != WFA_SUCCESS)
                DPRINT_WARNING(WFA_WNG, "start p2p dhcp client fail\n");
        }
    } else {
        if (shell(DHCPRESET) != WFA_SUCCESS)
            DPRINT_WARNING(WFA_WNG, "reset p2p dhcp fail\n");

        if (shell("ifconfig %s 0.0.0.0", ifname) != WFA_SUCCESS)
            DPRINT_WARNING(WFA_WNG, "set p2p interface down fail\n");
    }
}

static void *p2p_event_receive(void* data)
{
    struct wpa_ctrl *ctrl;
    int i;
    struct timeval tv;
    char buf[256];
    int go = 0;
    int fd, ret;
    fd_set rfd;
    size_t len;
    char *pos;
    char *ifname = (char*) data;
    const char *events[] = {
            "P2P-GROUP-STARTED",
            "P2P-GROUP-REMOVED",
            "P2P-INVITATION-RECEIVED",
            NULL
    };

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    ctrl = open_wpa_mon(ifname);
    if (!ctrl) {
        DPRINT_WARNING(WFA_WNG, "open wpa mon for p2p0 fail\n");
        goto exit;
    }

    fd = wpa_ctrl_get_fd(ctrl);
    if (fd < 0) {
        DPRINT_WARNING(WFA_WNG, "get fd fail for wpa ctrl\n");
        goto exit;
    }

    while (!gDut.p2p_params.stop_event_thread) {
        FD_ZERO(&rfd);
        FD_SET(fd, &rfd);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(fd + 1, &rfd, NULL, NULL, &tv);
        if (ret == 0)
            continue;
        if (ret < 0) {
            DPRINT_WARNING(WFA_WNG, "select fail, %s\n", strerror(errno));
            wUSLEEP(100000);
            continue;
        }

        len = sizeof(buf);
        if (wpa_ctrl_recv(ctrl, buf, &len) < 0) {
            DPRINT_WARNING(WFA_WNG, "fail waiting for events\n");
            continue;
        }
        if (len >= sizeof(buf))
            len = sizeof(buf) - 1;
        buf[len] = '\0';

        ret = 0;
        pos = strchr(buf, '>');
        if (pos) {
            for (i = 0; events[i]; i++) {
                if (strncmp(pos + 1, events[i], strlen(events[i])) == 0) {
                    ret = 1;
                    break; /* Event found */
                }
            }
        }
        if (!ret)
            continue;

        /*
         * output format:
         * <3>P2P-GROUP-STARTED p2p0 GO ssid="DIRECT-lF6.1.11" freq=2462 passphrase="ViGQq96f" go_dev_addr=02:08:22:5e:8d:55 [PERSISTENT]
         */

        DPRINT_INFO(WFA_OUT, "%s(), receive event: %s\n", __func__, buf);
        if (strstr(buf, "P2P-GROUP-")) {
            char *group_type, *pos, *ssid, *tmp;

            pos = strchr(buf, ' ');
            if (!pos)
                continue;
            *pos++ = '\0';

            tmp = pos;
            pos = strchr(pos, ' ');
            if (!pos)
                continue;
            *pos++ = '\0';

            wMEMSET(gDut.p2p_params.p2p_ifname, '\0', IFNAMSIZ);
            wSTRNCPY(gDut.p2p_params.p2p_ifname, tmp, IFNAMSIZ-1);
            DPRINT_INFO(WFA_OUT,"%s(), Interface :%s[%d]\n", __func__, gDut.p2p_params.p2p_ifname, strlen(gDut.p2p_params.p2p_ifname));

            group_type = pos;
            pos = strchr(group_type, ' ');
            if (!pos)
                continue;
            *pos++ = '\0';
            go = strcmp(group_type, "GO") == 0;
            DPRINT_INFO(WFA_OUT, "%s(), group_type: %s\n", __func__,
                    group_type);

            /* save ssid for getpsk */
            ssid = strstr(pos, "ssid=\"");
            if (ssid != NULL && go) {
                ssid += 6;
                pos = strchr(ssid, '"');
                if (pos != NULL) {
                    *pos++ = '\0';
                    wSTRNCPY(gDut.p2p_params.current_ssid, ssid,
                            WFA_SSID_NAME_LEN);
                    DPRINT_INFO(WFA_OUT, "%s(), current_ssid: %s\n",
                            __func__,
                            gDut.p2p_params.current_ssid);
                }
            }
        }

        if (strstr(buf, "P2P-GROUP-STARTED")) {
#ifdef CONFIG_MTK_WFD
            gDut.p2p_params.group_started = 1;
            gDut.p2p_params.role = go;
#endif
            enable_dhcp(gDut.p2p_params.p2p_ifname, 1, go);
        } else if (strstr(buf, "P2P-GROUP-REMOVED")) {
            enable_dhcp(gDut.p2p_params.p2p_ifname, 0, go);
            go = 0;
            wMEMSET(gDut.p2p_params.current_ssid, '\0', WFA_SSID_NAME_LEN);
            wMEMSET(gDut.p2p_params.p2p_ifname, '\0', IFNAMSIZ);
#ifdef CONFIG_MTK_WFD
            gDut.p2p_params.group_started = 0;
            gDut.p2p_params.role = -1;
#endif
        }

        /*
            output format:
            P2P-INVITATION-RECEIVED sa=02:10:18:96:2d:17
            go_dev_addr=02:10:18:96:2d:17 bssid=02:10:18:96:ad:17 unknown-network
        */

        if (strstr(buf, "P2P-INVITATION-RECEIVED")) {
            DPRINT_INFO(WFA_OUT, "P2P-INVITATION-RECEIVED!!\n");
            DPRINT_INFO(WFA_OUT, "wps method: %d\n", gDut.p2p_params.wps_method);
            DPRINT_INFO(WFA_OUT, "pin: %s\n", gDut.p2p_params.wpsPin);

            char cmdbuf[WFA_BUFF_1K];
            char cmd[WFA_BUFF_1K];
            char dev_addr[20]; //store go_dev_addr device address
            char *tmp = NULL;

            tmp = strtok(buf," ");
            tmp = strtok(NULL," ");
            /* Handle sa=02:10:18:96:2d:17 */

            tmp = strtok(NULL," ");
            /* Handle go_dev_addr=02:10:18:96:2d:17*/
            tmp = strtok(tmp,"=");
            tmp = strtok(NULL,"=");
            DPRINT_INFO(WFA_OUT, "%s\n", tmp);

            //store go_dev_addr
            dev_addr[0] = '\0';
            wSTRNCPY(dev_addr, tmp, sizeof(dev_addr));

            if (gDut.p2p_params.wps_method == WPS_METHOD_KEYPAD) {
                wSNPRINTF(cmd, sizeof(cmd), "P2P_CONNECT %s %s keypad join", dev_addr, gDut.p2p_params.wpsPin);
            }
            else {
                DPRINT_ERR(WFA_ERR, "recv P2P-INVITATION-RECEIVED!! TODO!\n");
                continue;
            }
            if (wpa_command(ifname, cmd) < 0) {
                DPRINT_ERR(WFA_ERR, "p2p connect fail\n");
            }
        }
    }

exit:
    enable_dhcp(ifname, 0, go);
    if (ctrl) {
        wpa_ctrl_detach(ctrl);
        wpa_ctrl_close(ctrl);
    }
    pthread_exit(0);
    return NULL;
}

static void remove_p2p_persistent_networks(const char *ifname)
{
    char buf[4096];
    char cmd[256];
    char *pos;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (wpa_command_resp(ifname, "LIST_NETWORKS", buf, sizeof(buf)) < 0)
        return;

    /*
     * output format:
     * network id / ssid / bssid / flags
     * 1       DIRECT-rB7.1.2  02:08:22:fe:8b:51       [DISABLED][P2P-PERSISTENT]
     */

    pos = strchr(buf, '\n');
    if (pos == NULL)
        return;
    pos++;
    while (pos && pos[0]) {
        int id = atoi(pos);
        wSNPRINTF(cmd, sizeof(cmd), "REMOVE_NETWORK %d", id);
        if (wpa_command(ifname, cmd) < 0) {
            DPRINT_ERR(WFA_ERR, "wpa cmd fail: %s\n", cmd);
            continue;
        }
        pos = strchr(pos, '\n');
        if (pos)
            pos++;
    }
}
#endif

void dump_wmm_paraters()
{
    struct wmm_param {
        char *name;
        char *key;
    };
    struct wmm_param wmm_params[] = {
        {"BK", "0x820f31a0"},
        {"BE", "0x820f31a4"},
        {"VI", "0x820f31a8"},
        {"VO", "0x820f31ac"},
        {"TXOP[VO|VI]", "0x820f4010"},
        {"TXOP[BE|BK]", "0x820f4014"},
    };
    int i = 0;
    int length = sizeof(wmm_params) / sizeof(struct wmm_param);

    switch (gDut.driver_gen) {
    case GEN_4m:
        /* Enable driver dump */
        shell(IWPRIV" wlan0 driver 'set_mcr 2011 2011'");
        for (i = 0; i < length; i++) {
            if (shell(IWPRIV" wlan0 driver 'get_mcr %s'", wmm_params[i].key) ==
                    WFA_SUCCESS) {
                DPRINT_INFO(WFA_OUT, "%s: %s\n", wmm_params[i].name,
                        gDut.shellResult);
            } else {
                DPRINT_WARNING(WFA_WNG, "get wmm parameter fail for %s\n",
                        wmm_params[i].name);
            }
        }
        break;
    default:
        break;
    }
}

#if defined(_FREERTOS)
int get_ip_config(const char* intf, caStaGetIpConfigResp_t *ifinfo)
{
    uint8_t tmp_ip[4] = {0};
    uint8_t tmp_mac[WFA_MAC_ADDR_STR_LEN] = {0};

    wifi_config_get_ip_addr(WIFI_PORT_STA, ifinfo->ipaddr, 0);

    wifi_config_get_ip_addr(WIFI_PORT_STA, ifinfo->mask, 1);

    wSNPRINTF(ifinfo->dns[0], WIFI_IP_BUFFER_LENGTH, "8.8.8.8");

    wSNPRINTF(ifinfo->dns[1], WIFI_IP_BUFFER_LENGTH, "8.8.4.4");

    get_mac(intf, (unsigned char*)tmp_mac);
    wSTRNCPY(ifinfo->mac, tmp_mac, sizeof(ifinfo->mac) - 1);
    return WFA_SUCCESS;
}
#else
int get_ip_config(const char* intf, caStaGetIpConfigResp_t *ifinfo)
{
    char *buf = NULL;
    char *buf_save = NULL;

    if (shell(GETIPCONFIG" %s", intf) != WFA_SUCCESS)
        return WFA_FAILURE;

    wMEMSET(ifinfo, 0, sizeof(*ifinfo));
    buf = strtok_r(gDut.shellResult, "\r\n", &buf_save);
    while (buf) {
        if (!wSTRNCMP(buf, "mac_addr=", 9)) {
            wSTRNCPY(ifinfo->mac, buf + 9, sizeof(ifinfo->mac) - 1);
        } else if (!wSTRNCMP(buf, "ip=", 3)) {
            wSTRNCPY(ifinfo->ipaddr, buf + 3, sizeof(ifinfo->ipaddr) - 1);
        } else if (!wSTRNCMP(buf, "mask=", 5)) {
            wSTRNCPY(ifinfo->mask, buf + 5, sizeof(ifinfo->mask) - 1);
        } else if (!wSTRNCMP(buf, "dns1=", 5)) {
            wSTRNCPY(ifinfo->dns[0], buf + 5, sizeof(ifinfo->dns[0]) - 1);
        } else if (!wSTRNCMP(buf, "dns2=", 5)) {
            wSTRNCPY(ifinfo->dns[1], buf + 5, sizeof(ifinfo->dns[1]) - 1);
        }
        buf = strtok_r(NULL, "\r\n", &buf_save);
    }
    return WFA_SUCCESS;
}
#endif

#ifdef _FREERTOS
int get_mac(const char* intf, unsigned char* addr)
{
    int ret = -1;
    int i = 0;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    wMEMSET(addr, 0, WFA_MAC_ADDR_LEN);

    if(wSTRNCMP("wlan0", intf, strlen("wlan0")) == 0)
	wifi_config_get_mac_address(WIFI_PORT_STA, addr);
    else
	DPRINT_INFO(WFA_OUT, "%s: not support %s get_mac addr\n",
			__func__, intf);

    DPRINT_INFO(WFA_OUT, "%s: get_mac addr: \n", __func__);
    for(i = 0; i < WIFI_MAC_ADDRESS_LENGTH; i++) {
	    DPRINT_INFO(WFA_OUT, "%02x ", addr[i]);
    }
    DPRINT_INFO(WFA_OUT, "\n");
    return STATUS_COMPLETE;
}
#else
int get_mac(const char* intf, unsigned char* addr)
{
    int s;
    struct ifreq ifr;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    wMEMSET(addr, 0, 6);
    s = wSOCKET(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        DPRINT_ERR(WFA_ERR, "socket create fail.\n");
        return STATUS_ERROR;
    }
    wMEMSET(&ifr, 0, sizeof(ifr));
    wSTRNCPY(ifr.ifr_name, intf, sizeof(ifr.ifr_name) - 1);
    if (wIOCTL(s, SIOCGIFHWADDR, &ifr) < 0) {
        wCLOSE(s);
        DPRINT_ERR(WFA_ERR, "ioctl get addr fail.\n");
        return STATUS_ERROR;
    }
    wMEMCPY(addr, ifr.ifr_hwaddr.sa_data, 6);
    wCLOSE(s);

    return STATUS_COMPLETE;
}
#endif

void mbo_set_non_pref_chan(const char* intf, struct mbo_non_pref_channel* chans)
{
    char buf[1024];

    if (!chans) {
        wMEMSET(gDut.sta_params.nonPrefChanStr, 0, sizeof(gDut.sta_params.nonPrefChanStr));
        wSNPRINTF(buf, sizeof(buf), "SET non_pref_chan ");
    } else {
        wSNPRINTF(buf, sizeof(buf), " %d:%d:%d:%d",
            chans->chOpClass, chans->chPrefNum,
            chans->chPref, chans->chReasonCode);
        strncat(gDut.sta_params.nonPrefChanStr, buf, sizeof(gDut.sta_params.nonPrefChanStr) - 1);
        wSNPRINTF(buf, sizeof(buf), "SET non_pref_chan%s", gDut.sta_params.nonPrefChanStr);
    }

    if (wpa_command(intf, buf) < 0) {
        DPRINT_INFO(WFA_ERR, "set non_pref_chan fail %s\n", gDut.sta_params.nonPrefChanStr);
    }
}

void mbo_set_cellular_data_cap(const char* intf, int cellularDataCap)
{
    char buf[1024];

    if (cellularDataCap > 0 && cellularDataCap < 4) {
        wSNPRINTF(buf, sizeof(buf), "SET mbo_cell_capa %d", cellularDataCap);
        if (wpa_command(intf, buf) < 0)
            DPRINT_INFO(WFA_ERR, "set mbo_cell_capa fail %d\n", cellularDataCap);
    } else {
        DPRINT_INFO(WFA_ERR, "Invalid mbo_cell_capa %d\n", cellularDataCap);
    }
}

int sta_scan_channel(const char* intf, int channel)
{
    char buf[1024];
    unsigned int freq = 0;

    if (channel) {
        if (channel >= 1 && channel <= 13)
            freq = 2407 + 5 * channel;
        if (channel == 14)
            freq = 2484;
        if (channel >= 36 && channel <= 165)
            freq = 5000 + 5 * channel;

        if (!freq) {
            DPRINT_INFO(WFA_ERR, "Invalid channel %d, freq %d\n", channel, freq);
            goto done;
        }
        wSNPRINTF(buf, sizeof(buf), "SCAN TYPE=ONLY freq=%d", freq);

        if (wpa_command(intf, buf) < 0) {
            DPRINT_INFO(WFA_ERR, "Scan failed\n");
            goto done;
        }
        sta_mon_event(intf, "CTRL-EVENT-SCAN-RESULTS");
    }
done:
    return WFA_SUCCESS;
}

#ifdef CONFIG_MTK_WFD
static void *wfaStaWaitingWfdConnection_AutoGO(void *data)
{

    int count;
    char cmd[256];
    char buf[1024];
    char peerIpStr[32] = {'\0'};
    char peerMacStr[32] = {'\0'};
    char sessionId[32] = {'\0'};
    char *peerMac;
    char *ifname = (char*)data;
    struct wpa_ctrl *ctrl;
    int result = 0;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    #define WAIT_TIME 30
    DPRINT_INFO(WFA_OUT, "Wait %ds for UCC to start WFD connecton...\n", WAIT_TIME);

    wSLEEP(WAIT_TIME);

    DPRINT_INFO(WFA_OUT, "Waiting CLIENT to connect...\n");

    ctrl = open_wpa_mon(ifname);
    if (ctrl == NULL) {
        DPRINT_ERR(WFA_ERR, "open wpa mon fail.\n");
        goto exit;
    }

    result = get_wpa_cli_event(ctrl, "AP-STA-CONNECTED", buf, sizeof(buf));

    wpa_ctrl_detach(ctrl);
    wpa_ctrl_close(ctrl);

    if (result < 0)
    {
         DPRINT_ERR(WFA_ERR, "P2P CLIENT doesn't connect before timeout!!\n");
         goto exit;
    }

    DPRINT_INFO(WFA_OUT, "STA connect event: %s\n", buf);
    peerMac = strstr(buf, "p2p_dev_addr=");

    if (peerMac == NULL)
    {
        DPRINT_ERR(WFA_ERR, "Cound not find Peer Mac address!!\n");
        goto exit;
    }

    sscanf(peerMac, "p2p_dev_addr=%s", peerMacStr);

    DPRINT_INFO(WFA_OUT, "Peer Mac: %s\n", peerMacStr);

    mtkWfdP2pConnect(&peerMacStr[0], NULL, 0);

exit:
    DPRINT_INFO(WFA_OUT, "Exit %s()\n", __func__);
    pthread_exit(NULL);
    return NULL;

}

static void *wfaStaWaitingWfdConnection_Nego(void *data)
{
    caStaP2pStartGrpForm_t *staP2pStartGrpForm = (caStaP2pStartGrpForm_t *)data;

    int localRespLen = 0;
    dutCmdResponse_t localInfoResp = {0};
    char localrespBuf[WFA_BUFF_4K] = {0};
    char devAddr[18] = {'\0'};
    char ssid[34] = {'\0'};
    int ret;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    DPRINT_INFO(WFA_OUT, "Starting P2P connection to peer %s\n", staP2pStartGrpForm->devId);
    memset((void *)localrespBuf, 0, WFA_BUFF_4K);
    memset((void *)&localInfoResp, 0, sizeof(dutCmdResponse_t));

    ret = wfaStaP2pStartGrpFormation(sizeof(caStaP2pStartGrpForm_t), (BYTE *)staP2pStartGrpForm,
                                    &localRespLen, (BYTE *)&localrespBuf[0]);
     if ((ret == STATUS_COMPLETE) &&
         (wfaGetTLVvalue(sizeof(dutCmdResponse_t), (BYTE *)&localrespBuf[0], &localInfoResp) == WFA_SUCCESS) &&
         strlen(localInfoResp.cmdru.grpFormInfo.grpId) != 0)
     {

         DPRINT_INFO(WFA_OUT, "P2P formation Success!!\n");

         sscanf(localInfoResp.cmdru.grpFormInfo.grpId, "%[0-9a-z:] %s", devAddr, ssid);

         DPRINT_INFO(WFA_OUT, "P2P connected to Addr: %s, SSID:%s\n", devAddr,ssid);
         DPRINT_INFO(WFA_OUT, "I'm P2P %s\n", localInfoResp.cmdru.grpFormInfo.result);

         mtkWfdP2pConnect(&devAddr, NULL, 0);
    }
    else
    {
         DPRINT_INFO(WFA_OUT, "P2P formation Fail!!\n");
    }

    DPRINT_INFO(WFA_OUT, "Exit %s()\n", __func__);
    pthread_exit(NULL);
    return NULL;
}

static int *mtkWfdP2pConnect(char *p2pPeerMac, char *sessionId, int sessionIdLen)
{
    char cmd[256];
    char buf[1024];
    char *wfdSubElems = NULL;
    char rtspPortStr[16] = {'\0'};
    long rtspPort;
    char devInfoStr[16] = {'\0'};
    long devInfo;
    int sessionAvail = 0;
    char peerIpStr[24] = {'\0'};
    char sessionIdStr[32] = {'\0'};
    char peerMacStr[24] = {'\0'};
    char peerMac2Str[24] = {'\0'};
    char ifname[IFNAMSIZ] = {'\0'};
    int count = 0;
    int result = 0;

    DPRINT_INFO(WFA_OUT, "Entering %s()\n", __func__);

    if (p2pPeerMac == NULL)
    {
        DPRINT_ERR(WFA_ERR,"p2pPeerMac is NULL!!\n");
        result = -1;
        goto Exit;
    }

    if (!gDut.p2p_params.group_started || (gDut.p2p_params.role == -1))
    {
        DPRINT_ERR(WFA_ERR, "P2P GROUP is not started(group=%d, role=%d), unable to establish WFD connection!!\n",
            gDut.p2p_params.group_started, gDut.p2p_params.role);
        result = -1;
        goto Exit;
    }

    DPRINT_INFO(WFA_OUT, "We are P2P-%s!!\n", (gDut.p2p_params.role == 0)?"CLIENT":"GO");

    strncpy(ifname, gDut.p2p_params.p2p_ifname, IFNAMSIZ - 1);
    ifname[IFNAMSIZ - 1] = '\0';

    strncpy(peerMacStr, p2pPeerMac, WFA_P2P_DEVID_LEN - 1);
    peerMacStr[WFA_P2P_DEVID_LEN - 1] = '\0';

#define MAX_TRY 120
    count = MAX_TRY;
    do
    {
        wSLEEP(1);
        if (gDut.p2p_params.role == 0) //GC
        {
            DPRINT_INFO(WFA_OUT, "Checking for DHCP Server IP...%d\n", count);
            if (shell(DHCPGETSERVERIP) == WFA_SUCCESS)
            {

                if (strlen(gDut.shellResult) != 0)
                {
                    strncpy(peerIpStr, gDut.shellResult, sizeof(peerIpStr));
                    break;
                }
            }
            else
            {
                DPRINT_ERR(WFA_ERR, "Unable to execute "DHCPGETSERVERIP"\n");
            }

        }
        else
        if (gDut.p2p_params.role == 1) // GO
        {

            //Assume only one client is connected and that one is what we want
            DPRINT_INFO(WFA_OUT, "Checking for DHCP client IP.....%d\n", count);
            if (shell(DHCPGETCLIENTIP) == WFA_SUCCESS)
            {
                if (strlen(gDut.shellResult) != 0)
                {
                    sscanf(gDut.shellResult, "%17s %15s", peerMac2Str, peerIpStr);
                    break;
                }
            }
            else
            {
                    DPRINT_ERR(WFA_ERR, "Unable to execute "DHCPGETCLIENTIP"\n");
            }
        }
        else // P2P connection is disconnected ??
        {
            DPRINT_ERR(WFA_ERR, "P2P GROUP is removed!!\n");
            result = -1;
            break;
        }

        count--;
    } while (count > 0);

    if (count == 0)
    {
        DPRINT_ERR(WFA_ERR, "Fail to get Peer IP!!\n");
        result = -1;
        goto Exit;
    }
    DPRINT_INFO(WFA_OUT, "The Peer IP is %s(%s)\n", peerIpStr,peerMac2Str);

    // get WFD information
    memset(cmd, '\0', sizeof(cmd));
    wSNPRINTF(cmd, sizeof(cmd), "P2P_PEER %s", peerMacStr);
    DPRINT_INFO(WFA_OUT, "ifname = %s\n", ifname);
    DPRINT_INFO(WFA_OUT, "cmd = %s\n", cmd);
    if (wpa_command_resp(ifname, cmd, buf, sizeof(buf)) < 0)
    {
        DPRINT_ERR(WFA_ERR, "Run cmd \'%s\' fail!!\n", cmd);
        result = -1;
        goto Exit;
    }

    if (strncmp(buf, "FAIL", 4) == 0)
    {
        DPRINT_ERR(WFA_ERR, "Unable to find peer %s!!\n", peerIpStr);
        result = -1;
        goto Exit;
    }

    wfdSubElems = strstr(buf, "wfd_subelems=");

    if (strlen(wfdSubElems) == 0)
    {
      DPRINT_ERR(WFA_ERR, "Unable to find wfd_subelems in peer %s!!\n", peerMacStr);
      result = -1;
      goto Exit;
    }

    wfdSubElems = strstr(wfdSubElems, "=") + 1;

#define OFFSET_OF_RTSTPORT 10
    memcpy(rtspPortStr, wfdSubElems + OFFSET_OF_RTSTPORT, 4);
    rtspPort = strtol(rtspPortStr, NULL, 16);

    if (rtspPort == 0)
    {
        DPRINT_ERR(WFA_ERR, "ERROR! RTSP port is 0, default to 7236\n");
        rtspPort = 7236;
    }
    snprintf(rtspPortStr, sizeof(rtspPortStr), "%d", rtspPort);

/*
    somehow MTK test bed remove sesson available bit at this time,
    remove checking for now
*/
#if 0
#define OFFSET_OF_DEVINFO 6
    memcpy(devInfoStr, wfdSubElems + OFFSET_OF_DEVINFO, 4);
    devInfo = strtol(devInfoStr, NULL, 16);
    DPRINT_INFO(WFA_OUT, "wfd defInfo = %02x\n", devInfo);

    sessionAvail = (devInfo & 0x30) > 1;
    DPRINT_INFO(WFA_OUT, "wfd session available = %d\n", sessionAvail);

    if (!sessionAvail)
    {
        DPRINT_ERR(WFA_ERR, "wfd session is not available!!\n");
        result = -1;
        goto Exit;
    }
#endif
    DPRINT_INFO(WFA_OUT, "Starting RTSP to [%s:%s]...\n", peerIpStr, rtspPortStr);

#define MAX_RTSP_RETRY 2
#define WAIT_BEFORE_RTSP    10
    int rtspRetry= 0;

    snprintf(rtspPortStr, sizeof(rtspPortStr), "%d", rtspPort);
    do
    {
        DPRINT_INFO(WFA_OUT, "Wait %ds before starting RTSP...\n", WAIT_BEFORE_RTSP);
        wSLEEP(WAIT_BEFORE_RTSP);

        if (!gDut.p2p_params.group_started)
        {
            DPRINT_ERR(WFA_ERR, "P2P group is removed!!\n");
            result = -1;
            break;
        }
        DPRINT_INFO(WFA_OUT, "Starting RTSP to [%s:%s]...\n", peerIpStr, rtspPortStr);
        if (wfaMtkWfdCmd_rtspStart(peerIpStr, rtspPortStr, sessionIdStr) == 0)
        {
            /* successful */
            DPRINT_INFO(WFA_OUT, "RTSP completed, session_id=[%s]\n", sessionIdStr);
            break;
        }
        else
        {
            /* failed */
            DPRINT_INFO(WFA_ERR, "RTSP negotiation is failed\n");

        }
            rtspRetry++;
            DPRINT_INFO(WFA_OUT, "Retrying RTSP (retry=%d, max=%d)...\n", rtspRetry, MAX_RTSP_RETRY);
    } while (rtspRetry < MAX_RTSP_RETRY);

    if (rtspRetry== MAX_RTSP_RETRY)
    {
        result = -1;
    }
Exit:
    if (result != 0)
    {
        DPRINT_INFO(WFA_OUT, "------------------------\n");
        DPRINT_INFO(WFA_OUT, "WFD Connection Failed!!!\n");
        DPRINT_INFO(WFA_OUT, "------------------------\n");
    }
    else
    {
        DPRINT_INFO(WFA_OUT, "------------------------\n");
        DPRINT_INFO(WFA_OUT, "WFD Connection Success!!!\n");
        DPRINT_INFO(WFA_OUT, "------------------------\n");
        if ((sessionId != NULL) && (sessionIdLen != 0))
        {
            strncpy(sessionId, sessionIdStr, sessionIdLen);
        }
    }


    return result;
}

#endif

void setTxPPDUEnable(int enabled)
{
    DPRINT_INFO(WFA_OUT, "setTxPPDUEnable %d\n", enabled);
    shell(IWPRIV " %s driver \"tx_ppdu %d\"", WFA_STAUT_IF, enabled);
}
