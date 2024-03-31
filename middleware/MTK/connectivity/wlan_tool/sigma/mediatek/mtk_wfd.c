#ifdef CONFIG_MTK_WFD
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#ifndef _FREERTOS
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/wireless.h>
#include <poll.h>
#else
#include <fcntl.h>
#include "lwip/sockets.h"
#include "lwip/inet.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "wfa_portall.h"
#include "wfa_debug.h"
#include "wfa_main.h"
#include "wfa_types.h"
#include "wfa_sock.h"
#include "wfa_tg.h"
#include "wfa_stdincs.h"
#include "wfa_cmds.h"
int wfaMtkWfdSendCmd(int *sockfd, char *cmd, char *rsp, int *rlen);
int wfaMtkWfdInitSock(void);
int wfaMtkWfdCmd_init(void);
int wfaMtkWfdCmd_rtspPlay(void);
int wfaMtkWfdCmd_rtspPause(void);
int wfaMtkWfdCmd_rtspTeardown(void);
int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);
int wfaMtkWfdCmdRspProcess(int curr_cmd, char *rsp, int rsp_len, char *ret_buf);
static void wfaMtkWfdDump(char *buf, int len);

extern unsigned short wfa_defined_debug;

#define MTK_WFD_PORT            2472
#define MTK_WFD_SND_BUF_SIZE    1024
#define MTK_WFD_RCV_BUF_SIZE    1024

#define WFD_SIGMA_CMD_SIZE      sizeof(struct wfd_sigma_cmd_hdr)

/* cmd list */
enum
{
    MTK_WFD_SIGMA_CMD_INIT = 0,
    MTK_WFD_SIGMA_CMD_RTSP_START, /* Start RTSP */
    MTK_WFD_SIGMA_CMD_RTSP_PLAY, /* M7 */
    MTK_WFD_SIGMA_CMD_RTSP_PAUSE, /* M10 */
    MTK_WFD_SIGMA_CMD_RTSP_TEARDOWN, /* M9 */
    MTK_WFD_SIGMA_CMD_RTSP_SEND_IDR_REQ, /* 5 */
    MTK_WFD_SIGMA_CMD_RSP_OK, /* response OK */
    MTK_WFD_SIGMA_CMD_RSP_ERROR_UNKNOWN, /* response Unknown error */
    MTK_WFD_SIGMA_CMD_RTSP_ENTER_STANDBY,
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_SINGLE,
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_MULTI, /* 10 */
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_CAP_UPDATE,
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_HIDC_EVENT_KEYBOARD,
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_HIDC_EVENT_MOUSE,
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_ENABLE_GENERIC,
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_ENABLE_HIDC, /* 15 */
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_KEYBOARD,
    MTK_WFD_SIGMA_CMD_RTSP_SET_WFD_DEV_TYPE,
    MTK_WFD_SIGMA_CMD_RTSP_ENABLE_HDCP2X,
    MTK_WFD_SIGMA_CMD_RTSP_SET_VIDEO_FORMAT,
    MTK_WFD_SIGMA_CMD_RTSP_SET_AUDIO_FORMAT,
    MTK_WFD_SIGMA_CMD_RTSP_CONF_RESET,
    MTK_WFD_SIGMA_CMD_SET_SESSION_AVAIL,
    MTK_WFD_SIGMA_CMD_DISABLE_ALL,
    MTK_WFD_SIGMA_CMD_RTSP_ENABLE_STANDBY,
    MTK_WFD_SIGMA_CMD_RTSP_ENABLE_I2C,
    MTK_WFD_SIGMA_CMD_RTSP_ENABLE_EDID,
    MTK_WFD_SIGMA_CMD_MAX
};

enum
{
    WFD_VIDEO_FORMAT_TYPE_CEA = 0,
    WFD_VIDEO_FORMAT_TYPE_VESA,
    WFD_VIDEO_FORMAT_TYPE_HH
};


struct wfd_sigma_cmd_hdr
{
    int len; /* length of the whole cmd message */
    int type; /* type of the message */
};

struct wfd_sigma_cmd
{
    struct wfd_sigma_cmd_hdr hdr;
    char data[32];
};


static int mtkWfdSockFd = -1;
extern char ctrl_if[];

int wfaMtkWfdInit(void)
{
    int ret = 0;

    DPRINT_ERR(WFA_ERR, "%s\n", __FUNCTION__);

    if(mtkWfdSockFd == -1)
    {
        mtkWfdSockFd = wfaMtkWfdInitSock();
        if (mtkWfdSockFd < 0)
        {
            DPRINT_ERR(WFA_ERR, "fail to connect to peer\n");
            goto mtkwfd_out_err;
        }
    }

    DPRINT_ERR(WFA_ERR, "connection to MTK WFD Ok\n");

    //test code
    /*
    {
        char ip[24]="";
        char mac[24]= "";
        char mask[24] = "";
        wfaMtkWfdP2pGetIfConfig(mac, ip, mask);

        DPRINT_ERR(WFA_ERR, "P2P IP get, [mac=%s, ip=%s, mask=%s]\n", mac, ip, mask);
    }
    */

    /*
    DPRINT_ERR(WFA_ERR, "Wating for 5 sec to send cmd\n");
    sleep (5);
    */
#if 0
    DPRINT_ERR(WFA_ERR, "TEST CODE: Send init CMD to WFD APP\n");
    if (wfaMtkWfdCmd_init()== 0){
        DPRINT_ERR(WFA_ERR, "wfaMtkWfdCmd_init() Success!!\n");
    }
    else {
        DPRINT_ERR(WFA_ERR, "wfaMtkWfdCmd_init() Fail!!\n");
    }
#endif
   /*
    sleep(3);
    wfaMtkWfdCmd_rtspStart("192.168.100.300", "554");
    */

    return ret;

mtkwfd_out_err:
    if (mtkWfdSockFd != -1)
        close(mtkWfdSockFd);
    mtkWfdSockFd = -1;
    exit(1);

}

int wfaMtkWfdCmd_init(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_INIT;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
//    wfaMtkWfdDump(rbuf, rlen);

    return 0;

}


int wfaMtkWfdCmd_rtspConfReset(void)
{
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_CONF_RESET;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;

}

int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId)
{
    /* sending init cmd to mtk wfd */
    char buf[128];
    char rbuf[128];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    if (!ipaddr || !port || !sessionId)
    {
        DPRINT_ERR(WFA_ERR, "Error input [ipaddr] or [port] or [sessionId buffer not provided] \n");
        return -1;
    }
    memset(rbuf, 0, sizeof(rbuf));
    /* prepare the data of string "ip:port:" */
    sprintf(rbuf, "%s:%s:", ipaddr, port);

    memset(buf, 0, sizeof(buf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_START;
    /* start cmd has data which carries "ip:port:" string */
    strncpy(cmdP->data, rbuf, strlen(rbuf));
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(rbuf) + 1 ;
    memset(rbuf, 0, sizeof(rbuf));
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
        if (wfaMtkWfdCmdRspProcess(cmdP->hdr.type, rbuf, rlen, sessionId) != 0)
            return -1;
    }

    return 0;

}

int wfaMtkWfdCmd_rtspPlay(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_PLAY;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}


int wfaMtkWfdCmd_rtspPause(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_PAUSE;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Pause\n");
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}

int wfaMtkWfdCmd_rtspTeardown(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    DPRINT_INFO(WFA_OUT, "%s in .....\n",__FUNCTION__);


    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_TEARDOWN;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: teardown\n");
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}

int wfaMtkWfdCmd_rtspUibcCapUpdate(int type)
{
    /* sending init cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    switch (type)
    {
        case WFD_CAP_UIBC_MOUSE:
        {
            strcpy(cmdP->data, "Mouse");
            break;
        }

        case WFD_CAP_UIBC_KEYBOARD:
        {
            strcpy(cmdP->data, "Keyboard");
            break;
        }

        default:
        {
            return -1;
        }

    }

    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_CAP_UPDATE;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data) +1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: MTK_WFD_SIGMA_CMD_RTSP_UIBC_CAP_UPDATE\n");
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;

}


int wfaMtkWfdCmd_rtspSendIdrReq(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_SEND_IDR_REQ;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    return 0;
}

int wfaMtkWfdCmd_rtspUibcGenEvent(int evtType)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (evtType == eSingleTouchEvent || evtType == eMouseEvent)
        cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_SINGLE;
    else if (evtType == eMultiTouchEvent)
        cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_MULTI;
    else if (evtType == eKeyBoardEvent)
        cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_KEYBOARD;
    else
        /* default to single touch */
        cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_SINGLE;

    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    return 0;
}


int wfaMtkWfdCmd_rtspUibcHidcEvent(int hidType)
{
    /* sending init cmd to mtk wfd */
    char buf[254];
    char rbuf[254];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (hidType != eKeyBoardEvent)
    {
        DPRINT_ERR(WFA_ERR, "%s:Warning, hidType != Keyboard\n", __FUNCTION__);
    }

    if (hidType == eKeyBoardEvent)
        cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_HIDC_EVENT_KEYBOARD;
    else if (hidType == eMouseEvent)
        cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_HIDC_EVENT_MOUSE;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    return 0;
}

int wfaMtkWfdCmd_rtspUibcCapEnable(int cap_type)
{
    /* sending init cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    switch (cap_type)
    {
        case eUibcGen:
        {
            cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_ENABLE_GENERIC;
            break;
        }
        case eUibcHid:
        {
            cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_ENABLE_HIDC;
            break;
        }
        default:
        {
            DPRINT_ERR(WFA_ERR, "%s: Unknown cap_type, force use Generic\n", __FUNCTION__);
            cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_ENABLE_GENERIC;
            break;
        }
    }
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd\n");
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    return 0;
}

int wfaMtkWfdCmd_rtspEnterStandby(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_ENTER_STANDBY;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    return 0;
}


int wfaMtkWfdCmd_rtspSetWfdDevType(int wfdDevType)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    switch (wfdDevType)
    {
        case ePSink:
        {
            strcpy(cmdP->data, "P-Sink");
            break;
        }

        case eSSink:
        {
            strcpy(cmdP->data, "S-Sink");
            break;
        }

        default:
        {
            strcpy(cmdP->data, "P-Sink");
            break;
        }

    }

    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_SET_WFD_DEV_TYPE;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data)+1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: MTK_WFD_SIGMA_CMD_RTSP_SET_WFD_DEV_TYPE\n");
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;

}

int wfaMtkWfdCmd_rtspEnableHDCP2X(int enable)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (enable)
        strcpy(cmdP->data, "enable");
    else
        strcpy(cmdP->data, "disable");

    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_ENABLE_HDCP2X;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data)+1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}

int wfaMtkWfdCmd_rtspEnableSessionAvail(int enable)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (enable)
        strcpy(cmdP->data, "enable");
    else
        strcpy(cmdP->data, "disable");

    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_SET_SESSION_AVAIL;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data)+1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}



int wfaMtkWfdCmd_rtspSetVideoFormat(unsigned char *index_array, int array_size)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;


    /* video format bitmask */
    if (!index_array || !array_size)
    {
        DPRINT_ERR(WFA_ERR, "%s: Error video format array\n", __FUNCTION__);
        return -1;
    }
    cmdP->data[0] = (char)array_size;
    memcpy(&cmdP->data[1], index_array, array_size);
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_SET_VIDEO_FORMAT;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 1 + array_size;

    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}

int wfaMtkWfdCmd_rtspEnableStandby(int enable)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (enable)
        strcpy(cmdP->data, "enable");
    else
        strcpy(cmdP->data, "disable");

    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_ENABLE_STANDBY;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data)+1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}

int wfaMtkWfdCmd_rtspEnableI2c(int enable)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (enable)
        strcpy(cmdP->data, "enable");
    else
        strcpy(cmdP->data, "disable");

    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_ENABLE_I2C;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data)+1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}

int wfaMtkWfdCmd_rtspEnableEdid(int enable)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (enable)
        strcpy(cmdP->data, "enable");
    else
        strcpy(cmdP->data, "disable");

    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_ENABLE_EDID;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data)+1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}


int wfaMtkWfdCmd_sigmaDisableAll(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_DISABLE_ALL;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        DPRINT_INFO(WFA_OUT, "received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}


int wfaMtkWfdInitSock(void)
{
    int ret = 0;
    int sockfd = -1;
    struct sockaddr_in servAddr;

    if ((sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        DPRINT_ERR(WFA_ERR, "socket() failed: %i\n", errno);
        ret = -1;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port        = htons(MTK_WFD_PORT);

    DPRINT_ERR(WFA_ERR, "connecting to MTK WFD\n");

    if (connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
    {
        DPRINT_ERR(WFA_ERR, "connect() failed\n");
        close(sockfd);
        ret = -1;
    }

    if (ret < 0)
        return ret;
    else
        return sockfd;
}

static void wfaMtkWfdDump(char *buf, int len)
{
    int pos = 0;
    if (!buf)
        return;

    DPRINT_INFO(WFA_OUT, "Dumping buf (len=%d):\n---------", len);
    while(pos < len)
    {
        if (pos%8 == 0)
        {
            DPRINT_INFO(WFA_OUT, "\n%04d| ", pos);
        }
        DPRINT_INFO(WFA_OUT, "%02x ", buf[pos]);
        pos ++;
    }
    DPRINT_INFO(WFA_OUT, "\n---------\n");
}

int wfaMtkWfdSendCmd(int *sockfd, char *cmd, char *rsp, int *rlen)
{
    struct wfd_sigma_cmd *cmdP = NULL;
    int sndLen = 0, retLen = 0;
    fd_set sockSet;
    int maxfdn1 = -1;
    int nfds = 0;
    int ret = 0;

    DPRINT_INFO(WFA_OUT, "in ....\n", sndLen, retLen);

    if (*sockfd == -1)
    {
        DPRINT_ERR(WFA_ERR, "socket is not ready, trying to connect.\n");
        *sockfd = wfaMtkWfdInitSock();
        if (*sockfd < 0)
        {
            DPRINT_ERR(WFA_ERR, "fail to connect to peer\n");
            exit (1);
        }
        else
            DPRINT_ERR(WFA_ERR, "socket connected.\n");

    }

    cmdP = (struct wfd_sigma_cmd *)cmd;
    sndLen = cmdP->hdr.len;
    retLen = send(*sockfd, cmd, sndLen, 0);
    if (retLen != sndLen)
    {
        DPRINT_ERR(WFA_ERR, "sending len mistach (requested=%d, sent=%d)\n", sndLen, retLen);
        ret = -1;
        goto wfdsend_exit;
    }

    /* prepare to receive response */
    FD_ZERO(&sockSet);
    FD_SET(*sockfd, &sockSet);
    maxfdn1 = (*sockfd) + 1;

    DPRINT_ERR(WFA_ERR, "Waiting for response from MTK WFD\n");
    if((nfds = select(maxfdn1, &sockSet, NULL, NULL, NULL)) < 0)
    {
        DPRINT_ERR(WFA_ERR, "select error!\n");
        ret = -1;
        goto wfdsend_exit;
    }
    else
    {
        if ((retLen = recv(*sockfd, rsp, *rlen, 0)) <= 0)
        {
            DPRINT_ERR(WFA_ERR, "recv() failed or connection closed prematurely");
            ret = -1;
            goto wfdsend_exit;
        }
        *rlen = retLen;
        DPRINT_ERR(WFA_ERR, "Received response from MTK WFD (len=%d)\n", retLen);
    }
    ret = 0;

wfdsend_exit:

	DPRINT_ERR(WFA_ERR, "ret=%d\n", ret);
    return ret;
}

int wfaMtkWfdCmdRspProcess(int curr_cmd, char *rsp, int rsp_len, char *ret_buf)
{
    struct wfd_sigma_cmd *cmdRspP = (struct wfd_sigma_cmd *)rsp;
    char *ptr = NULL;

    if (!rsp || !rsp_len)
    {
        DPRINT_ERR(WFA_ERR, "No response message. Ignoring...\n");
        return -1;
    }
    switch(curr_cmd)
    {
        case (MTK_WFD_SIGMA_CMD_RTSP_START):
        {
            if (cmdRspP->hdr.type != MTK_WFD_SIGMA_CMD_RSP_OK)
            {
                DPRINT_ERR(WFA_ERR, "Cmd Response from MTK WFD is NG (err = %d)...\n", cmdRspP->hdr.type);
                return -1;
            }
            if (!ret_buf)
            {
                DPRINT_ERR(WFA_ERR, "%s: return buffer not supplied\n", __FUNCTION__);
                return -1;
            }
            /* parse to get the sessionId */
            /* the cmd format is
                "[session_id]:"
                where session_id is in hex or digits
            */
            ptr = strchr(cmdRspP->data, ':');
            if (!ptr)
            {
                DPRINT_ERR(WFA_ERR, "%s: session id not found int the response\n", __FUNCTION__);
                return -1;
            }
            strncpy(ret_buf, cmdRspP->data, ptr-cmdRspP->data);
            ret_buf[ptr-cmdRspP->data] = 0x00;
            return 0;
        }

        case (MTK_WFD_SIGMA_CMD_RTSP_PLAY):
        case (MTK_WFD_SIGMA_CMD_RTSP_PAUSE):
        case (MTK_WFD_SIGMA_CMD_RTSP_TEARDOWN):
        case (MTK_WFD_SIGMA_CMD_RTSP_ENTER_STANDBY):
        case (MTK_WFD_SIGMA_CMD_RTSP_SEND_IDR_REQ):
        case (MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_SINGLE):
        case (MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_KEYBOARD):
        case (MTK_WFD_SIGMA_CMD_RTSP_UIBC_HIDC_EVENT_KEYBOARD):
        case (MTK_WFD_SIGMA_CMD_RTSP_UIBC_HIDC_EVENT_MOUSE):
        case (MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_MULTI):
        {
            break;
            if (cmdRspP->hdr.type != MTK_WFD_SIGMA_CMD_RSP_OK)
            {
                DPRINT_ERR(WFA_ERR, "Cmd Response from MTK WFD is NG (err = %d)...\n", cmdRspP->hdr.type);
                return -1;
            }
            if (strlen(ret_buf))
                DPRINT_INFO(WFA_OUT, "Cmd Response =[%s]\n", ret_buf);
        }

        default:

            break;

    }
    return 0;

}

#endif
