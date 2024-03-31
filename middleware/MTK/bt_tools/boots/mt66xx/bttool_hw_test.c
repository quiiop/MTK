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
 * MediaTek Inc. (C) 2014. All rights reserved.
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

#include "FreeRTOS.h"
#include "autobt.h"
#include "bttool_hw_test.h"


/**************************************************************************
 *                 G L O B A L   V A R I A B L E S                        *
***************************************************************************/
static BOOL b_drv_open = FALSE;

typedef int (*OPTXPOWEROFFSET)(int wr_flag, char *group_offset, char *group_flag);
OPTXPOWEROFFSET    bt_op_tx_power_offset = NULL;

/**************************************************************************
  *                         F U N C T I O N S                             *
***************************************************************************/

int HW_TEST_BT_init(void)
{
    int ret = 0;

    BTTOOL_LOG_FUNC();

    if ((ret = bt_driver_power_on()) != 0) {
        BTTOOL_LOGE("open driver fail");
        return -1;
    }
    b_drv_open = TRUE;
    return 0;
}

void HW_TEST_BT_deinit(void)
{
    BTTOOL_LOG_FUNC();
    HW_TEST_BT_reset();  //no need to close driver, just reset and restore data ready cb
    b_drv_open = FALSE;
}

BOOL HW_TEST_BT_reset(void)
{
    UINT8 HCI_RESET[] = {0x01, 0x03, 0x0C, 0x0};
    UINT8 ucAckEvent[7];
    /* Event expected */
    UINT8 ucEvent[] = {0x04, 0x0E, 0x04, 0x01, 0x03, 0x0C, 0x00};
    UINT32 i;

    BTTOOL_LOG_FUNC();

    if (!b_drv_open) {
        BTTOOL_LOGE("bt driver is not open!");
        return FALSE;
    }

    if (bt_driver_tx(HCI_RESET, sizeof(HCI_RESET)) < 0) {
        BTTOOL_LOGE("Send HCI reset command fail");
        return FALSE;
    }

    bt_tool_dump_buffer("HCI RESET", HCI_RESET, sizeof(HCI_RESET));

    if (bt_driver_rx_timeout(ucAckEvent, sizeof(ucAckEvent)) < 0) {
        BTTOOL_LOGE("Receive command complete event fail");
        return FALSE;
    }

    bt_tool_dump_buffer("ACK EVENT", ucAckEvent, sizeof(ucAckEvent));

    if (memcmp(ucAckEvent, ucEvent, sizeof(ucEvent))) {
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }

    return TRUE;
}

BOOL HW_TEST_BT_TxOnlyTest_start(BT_HW_TEST_CMD_T *test_cmd)
{
    UINT8 HCI_VS_TX_TEST[] = {0x01, 0x0D, 0xFC, 0x17, 0x00,
                              0x00,
                              0x00, /* Tx pattern */
                              0x00, /* Single frequency or 79 channels hopping */
                              0x00, /* Tx channel */
                              0x00,
                              0x00, 0x01,
                              0x00, /* Packet type */
                              0x00, 0x00, /* Packet length */
                              0x02, 0x00, 0x01, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00};
    UINT8 ucAckEvent1[14];
    UINT8 ucAckEvent2[7];
    /* Event expected */
    UINT8 ucEvent1[] = {0x04, 0x03, 0x0B, 0x00}; /* Connection complete */
    UINT8 ucEvent2[] = {0x04, 0x0E, 0x04, 0x01, 0x0D, 0xFC, 0x00}; /* Command complete */
    UINT32 i;

    BTTOOL_LOG_FUNC();

    if (!b_drv_open) {
        BTTOOL_LOGE("bt driver is not open!");
        return FALSE;
    }

    /* Prepare Tx test command */
    HCI_VS_TX_TEST[6] = test_cmd->params.cmd_tx.tx_pattern;
    HCI_VS_TX_TEST[7] = test_cmd->params.cmd_tx.hopping;
    HCI_VS_TX_TEST[8] = (UINT8)test_cmd->params.cmd_tx.channel;
    HCI_VS_TX_TEST[12] = test_cmd->params.cmd_tx.packet_type;
    HCI_VS_TX_TEST[13] = (UINT8)(test_cmd->params.cmd_tx.packet_len & 0xFF);
    HCI_VS_TX_TEST[14] = (UINT8)((test_cmd->params.cmd_tx.packet_len >> 8) & 0xFF);

    if (bt_driver_tx(HCI_VS_TX_TEST, sizeof(HCI_VS_TX_TEST)) < 0) {
        BTTOOL_LOGE("Send Tx test command fail");
        return FALSE;
    }
    bt_tool_dump_buffer("HCI_VS_TX_TEST", HCI_VS_TX_TEST, sizeof(HCI_VS_TX_TEST));

    if (test_cmd->params.cmd_tx.tx_pattern != 0x0A) {
        /* Receive connection complete event */
        if (bt_driver_rx_timeout(ucAckEvent1, sizeof(ucAckEvent1)) < 0) {
            BTTOOL_LOGE("Receive connection complete event fail");
            return FALSE;
        }
        bt_tool_dump_buffer("RECV EVENT1", ucAckEvent1, sizeof(ucAckEvent1));

        if (memcmp(ucAckEvent1, ucEvent1, sizeof(ucEvent1))) {
            BTTOOL_LOGE("Receive unexpected event");
            return FALSE;
        }
    }

    /* Receive command complete event */
    if (bt_driver_rx_timeout(ucAckEvent2, sizeof(ucAckEvent2)) < 0) {
        BTTOOL_LOGE("Receive command complete event fail");
        return FALSE;
    }
    bt_tool_dump_buffer("RECV EVENT2", ucAckEvent2, sizeof(ucAckEvent2));

    if (memcmp(ucAckEvent2, ucEvent2, sizeof(ucEvent2))) {
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }

    return TRUE;
}

BOOL HW_TEST_BT_TxOnlyTest_end(void)
{
    //return HW_TEST_BT_reset();
    return TRUE;
}

BOOL HW_TEST_BT_NonSignalRx_start(BT_HW_TEST_CMD_T *test_cmd)
{
    UINT8 HCI_VS_RX_TEST[] = {0x01, 0x0D, 0xFC, 0x17, 0x00,
                              0x00, /* Rx pattern */
                              0x0B, /* Rx test mode */
                              0x00,
                              0x00,
                              0x00, /* Rx channel */
                              0x00, 0x01,
                              0x00, /* Packet type */
                              0x00, 0x00,
                              0x02, 0x00, 0x01, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Tester address */
                              0x00, 0x00};
    UINT8 ucAckEvent1[14];
    UINT8 ucAckEvent2[7];
    /* Event expected */
    UINT8 ucEvent1[] = {0x04, 0x03, 0x0B, 0x00}; /* Connection complete */
    UINT8 ucEvent2[] = {0x04, 0x0E, 0x04, 0x01, 0x0D, 0xFC, 0x00}; /* Command complete */
    UINT32 i;

    BTTOOL_LOG_FUNC();

    if (!b_drv_open) {
        BTTOOL_LOGE("bt driver is not open!");
        return FALSE;
    }

    /* Prepare Non-Signal-Rx test command */
    HCI_VS_RX_TEST[5] = test_cmd->params.cmd_nsrx.rx_pattern;
    HCI_VS_RX_TEST[9] = (UINT8)test_cmd->params.cmd_nsrx.channel;
    HCI_VS_RX_TEST[12] = test_cmd->params.cmd_nsrx.packet_type;
    HCI_VS_RX_TEST[21] = (UINT8)((test_cmd->params.cmd_nsrx.tester_addr >> 24) & 0xFF);
    HCI_VS_RX_TEST[22] = (UINT8)((test_cmd->params.cmd_nsrx.tester_addr >> 16) & 0xFF);
    HCI_VS_RX_TEST[23] = (UINT8)((test_cmd->params.cmd_nsrx.tester_addr >> 8) & 0xFF);
    HCI_VS_RX_TEST[24] = (UINT8)(test_cmd->params.cmd_nsrx.tester_addr & 0xFF);

    if (bt_driver_tx(HCI_VS_RX_TEST, sizeof(HCI_VS_RX_TEST)) < 0) {
        BTTOOL_LOGE("Send Non-Signal-Rx test command fail");
        return FALSE;
    }
    bt_tool_dump_buffer("HCI_VS_RX_TEST", HCI_VS_RX_TEST, sizeof(HCI_VS_RX_TEST));

    /* Receive connection complete event */
    if (bt_driver_rx_timeout(ucAckEvent1, sizeof(ucAckEvent1)) < 0) {
        BTTOOL_LOGE("Receive connection complete event fail");
        return FALSE;
    }
    bt_tool_dump_buffer("RECV EVENT1", ucAckEvent1, sizeof(ucAckEvent1));

    if (memcmp(ucAckEvent1, ucEvent1, sizeof(ucEvent1))) {
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }

    /* Receive command complete event */
    if (bt_driver_rx_timeout(ucAckEvent2, sizeof(ucAckEvent2)) < 0){
        BTTOOL_LOGE("Receive command complete event fail");
        return FALSE;
    }
    bt_tool_dump_buffer("RECV EVENT2", ucAckEvent2, sizeof(ucAckEvent2));

    if (memcmp(ucAckEvent2, ucEvent2, sizeof(ucEvent2))) {
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }

    return TRUE;
}

BOOL HW_TEST_BT_NonSignalRx_end(
    UINT32 *pu4RxPktCount,
    float  *pftPktErrRate,
    UINT32 *pu4RxByteCount,
    float  *pftBitErrRate
    )
{
    UINT8 HCI_VS_TEST_END[] = {0x01, 0x0D, 0xFC, 0x17, 0x00,
                               0x00,
                               0xFF, /* test end */
                               0x00,
                               0x00,
                               0x00,
                               0x00, 0x01,
                               0x00,
                               0x00, 0x00,
                               0x02, 0x00, 0x01, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00};
    UINT8 ucAckEvent[23];
    /* Event expected, the remaining bytes contain the test result */
    UINT8 ucEvent[] = {0x04, 0x0E, 0x14, 0x01, 0x0D, 0xFC, 0x00};
    UINT32 i;

    BTTOOL_LOG_FUNC();

    if (!b_drv_open) {
        BTTOOL_LOGE("bt driver is not open!");
        return FALSE;
    }

    /* Non-Signal-Rx test end command */
    if (bt_driver_tx(HCI_VS_TEST_END, sizeof(HCI_VS_TEST_END)) < 0) {
        BTTOOL_LOGE("Send test end command fail");
        return FALSE;
    }
    bt_tool_dump_buffer("HCI_VS_TEST_END", HCI_VS_TEST_END, sizeof(HCI_VS_TEST_END));

    /* Receive command complete event */
    if (bt_driver_rx_timeout(ucAckEvent, sizeof(ucAckEvent)) < 0) {
        BTTOOL_LOGE("Receive command complete event fail");
        return FALSE;
    }
    bt_tool_dump_buffer("RECV EVENT", ucAckEvent, sizeof(ucAckEvent));

    if (memcmp(ucAckEvent, ucEvent, sizeof(ucEvent))) {
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }
    else {
        /*
        * Parsing the test result:
        *   received packet count + PER + received payload byte count + BER
        */
        *pu4RxPktCount = *((UINT32*)&ucAckEvent[7]);
        *pftPktErrRate = (float)(*((UINT32*)&ucAckEvent[11]))/1000000;
        *pu4RxByteCount = *((UINT32*)&ucAckEvent[15]);
        *pftBitErrRate = (float)(*((UINT32*)&ucAckEvent[19]))/1000000;
    }

    return TRUE;
}


extern BOOL waiting_stop_cmd;

BOOL HW_TEST_BT_TestMode_Rx_loop(void)
{
    UINT8 ucRxBuf[512];
    UINT8 ucHeader = 0;
    UINT32 u4Len = 0, pkt_len = 0;
    UINT32 i;

    BTTOOL_LOG_FUNC();

    while (waiting_stop_cmd) {

        if (!b_drv_open) {
            BTTOOL_LOGE("bt driver is not open!");
            return FALSE;
        }

        if (bt_driver_rx_timeout(&ucHeader, sizeof(ucHeader)) < 0) {
            BTTOOL_LOGD("Zero byte read");
            continue;
        }

        memset(ucRxBuf, 0, sizeof(ucRxBuf));
        ucRxBuf[0] = ucHeader;
        u4Len = 1;

        switch (ucHeader) {
          case 0x04:
            BTTOOL_LOGD("Receive HCI event");
            if (bt_driver_rx_timeout(&ucRxBuf[1], 2) < 0) {
                BTTOOL_LOGE("Read event header fails");
                goto CleanUp;
            }

            u4Len += 2;
            pkt_len = (UINT32)ucRxBuf[2];
            if ((u4Len + pkt_len) > sizeof(ucRxBuf)) {
                BTTOOL_LOGE("Read buffer overflow! packet len %d", u4Len + pkt_len);
                goto CleanUp;
            }

            if (bt_driver_rx_timeout(&ucRxBuf[3], pkt_len) < 0) {
                BTTOOL_LOGE("Read event param fail");
                goto CleanUp;
            }

            u4Len += pkt_len;
            break;

          case 0x02:
            BTTOOL_LOGD("Receive ACL data");
            if (bt_driver_rx_timeout(&ucRxBuf[1], 4) < 0) {
                BTTOOL_LOGE("Read ACL header fails");
                goto CleanUp;
            }

            u4Len += 4;
            pkt_len = (((UINT32)ucRxBuf[4]) << 8);
            pkt_len += (UINT32)ucRxBuf[3]; /*little endian*/
            if ((u4Len + pkt_len) > sizeof(ucRxBuf)) {
                BTTOOL_LOGE("Read buffer overflow! packet len %d", u4Len + pkt_len);
                goto CleanUp;
            }

            if (bt_driver_rx_timeout(&ucRxBuf[5], pkt_len) < 0) {
                BTTOOL_LOGE("Read ACL data fails");
                goto CleanUp;
            }

            u4Len += pkt_len;
            break;

          case 0x03:
            BTTOOL_LOGD("Receive SCO data");
            if (bt_driver_rx_timeout(&ucRxBuf[1], 3) < 0) {
                BTTOOL_LOGE("Read SCO header fail");
                goto CleanUp;
            }

            u4Len += 3;
            pkt_len = (UINT32)ucRxBuf[3];
            if ((u4Len + pkt_len) > sizeof(ucRxBuf)) {
                BTTOOL_LOGE("Read buffer overflow! packet len %d", u4Len + pkt_len);
                goto CleanUp;
            }

            if (bt_driver_rx_timeout(&ucRxBuf[4], pkt_len) < 0) {
                BTTOOL_LOGE("Read SCO data fails");
                goto CleanUp;
            }

            u4Len += pkt_len;
            break;

          default:
            BTTOOL_LOGE("Unexpected BT packet header %02x", ucHeader);
            goto CleanUp;
        }

        /* Dump rx packet */
        bt_tool_dump_buffer("test mode rx", ucRxBuf, u4Len);
    }
    return TRUE;

CleanUp:
    return FALSE;
}

BOOL HW_TEST_BT_TestMode_enter(BT_HW_TEST_CMD_T *test_cmd)
{
    UINT8 HCI_VS_SET_RADIO[] =
        {0x01, 0x79, 0xFC, 0x06, 0x07, 0x80, 0x00, 0x06, 0x05, 0x07};
    UINT8 HCI_TEST_MODE_ENABLE[] =
        {0x01, 0x03, 0x18, 0x00};
    UINT8 HCI_WRITE_SCAN_ENABLE[] =
        {0x01, 0x1A, 0x0C, 0x01, 0x03};
    UINT8 HCI_SET_EVENT_FILTER[] =
        {0x01, 0x05, 0x0C, 0x03, 0x02, 0x00, 0x02};
    int power = test_cmd->params.cmd_tm_power;

    UINT8 ucAckEvent[7];
    /* Event expected */
    UINT8 ucEvent1[] = {0x04, 0x0E, 0x04, 0x01, 0x79, 0xFC, 0x00};
    UINT8 ucEvent2[] = {0x04, 0x0E, 0x04, 0x01, 0x03, 0x18, 0x00};
    UINT8 ucEvent3[] = {0x04, 0x0E, 0x04, 0x01, 0x1A, 0x0C, 0x00};
    UINT8 ucEvent4[] = {0x04, 0x0E, 0x04, 0x01, 0x05, 0x0C, 0x00};
    UINT32 i;

    BTTOOL_LOG_FUNC();

    if (!b_drv_open) {
        BTTOOL_LOGE("bt driver is not open!");
        return FALSE;
    }

    /*
    * First command: Set Tx power
    */

    if (power >= 0 && power <= 7) {
        HCI_VS_SET_RADIO[4] = (UINT8)power;
        HCI_VS_SET_RADIO[9] = (UINT8)power;

        if (bt_driver_tx(HCI_VS_SET_RADIO, sizeof(HCI_VS_SET_RADIO)) < 0) {
            BTTOOL_LOGE("Send set Tx power command fail");
            return FALSE;
        }
        bt_tool_dump_buffer("HCI_VS_SET_RADIO", HCI_VS_SET_RADIO, sizeof(HCI_VS_SET_RADIO));

        if (bt_driver_rx_timeout(ucAckEvent, sizeof(ucAckEvent)) < 0) {
            BTTOOL_LOGE("Receive command complete event fail");
            return FALSE;
        }
        bt_tool_dump_buffer("RECV EVENT", ucAckEvent, sizeof(ucAckEvent));

        if (memcmp(ucAckEvent, ucEvent1, sizeof(ucEvent1))){
            BTTOOL_LOGE("Receive unexpected event");
            return FALSE;
        }
    }

    /*
    * Second command: HCI_Enable_Device_Under_Test_Mode
    */

    if (bt_driver_tx(HCI_TEST_MODE_ENABLE, sizeof(HCI_TEST_MODE_ENABLE)) < 0) {
        BTTOOL_LOGE("Send test mode enable command fail");
        return FALSE;
    }
    bt_tool_dump_buffer("HCI_TEST_MODE_ENABLE", HCI_TEST_MODE_ENABLE, sizeof(HCI_TEST_MODE_ENABLE));

    if (bt_driver_rx_timeout(ucAckEvent, sizeof(ucAckEvent)) < 0) {
        BTTOOL_LOGE("Receive command complete event fail");
        return FALSE;
    }
    bt_tool_dump_buffer("RECV EVENT", ucAckEvent, sizeof(ucAckEvent));

    if (memcmp(ucAckEvent, ucEvent2, sizeof(ucEvent2))) {
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }

    /*
    * Third command: HCI_Write_Scan_Enable
    */
    if (bt_driver_tx(HCI_WRITE_SCAN_ENABLE, sizeof(HCI_WRITE_SCAN_ENABLE)) < 0) {
        BTTOOL_LOGE("Send write scan enable command fail");
        return FALSE;
    }
    bt_tool_dump_buffer("HCI_WRITE_SCAN_ENABLE", HCI_WRITE_SCAN_ENABLE, sizeof(HCI_WRITE_SCAN_ENABLE));

    if (bt_driver_rx_timeout(ucAckEvent, sizeof(ucAckEvent)) < 0) {
        BTTOOL_LOGE("Receive command complete event fail");
        return FALSE;
    }
    bt_tool_dump_buffer("RECV EVENT", ucAckEvent, sizeof(ucAckEvent));

    if(memcmp(ucAckEvent, ucEvent3, sizeof(ucEvent3))){
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }

    /*
    * Fourth command: HCI_Set_Event_Filter
    */
    if (bt_driver_tx(HCI_SET_EVENT_FILTER, sizeof(HCI_SET_EVENT_FILTER)) < 0) {
        BTTOOL_LOGE("Send set event filter command fail");
        return FALSE;
    }
    bt_tool_dump_buffer("HCI_SET_EVENT_FILTER", HCI_SET_EVENT_FILTER, sizeof(HCI_SET_EVENT_FILTER));

    if (bt_driver_rx_timeout(ucAckEvent, sizeof(ucAckEvent)) < 0) {
        BTTOOL_LOGE("Receive command complete event fail");
        return FALSE;
    }
    bt_tool_dump_buffer("RECV EVENT", ucAckEvent, sizeof(ucAckEvent));

    if (memcmp(ucAckEvent, ucEvent4, sizeof(ucEvent4))) {
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }
    return TRUE;
}

BOOL HW_TEST_BT_TestMode_exit(void)
{
    return TRUE;
}

BOOL HW_TEST_BT_parse_inq_resp(void)
{
    unsigned char ucRxBuf[512];
    unsigned char ucHeader = 0, retry = 0;
    unsigned int u4Len = 0, pkt_len = 0;
    unsigned char btaddr[6];
    char  str[512];
    char *p_str;
    int inq_complete = FALSE;

    while (!inq_complete) {
        if (bt_driver_rx_timeout(&ucHeader, sizeof(ucHeader)) < 0) {
            BTTOOL_LOGW("Zero byte read, retry = %d", retry);
            if(++retry == 5)
                goto CleanUp;
            continue;
        }

        memset(ucRxBuf, 0, sizeof(ucRxBuf));
        ucRxBuf[0] = ucHeader;
        u4Len = 1;

        switch (ucHeader) {
          case 0x04:
            BTTOOL_LOGD("Receive HCI event");
            if (bt_driver_rx_timeout(&ucRxBuf[1], 2) < 0) {
                BTTOOL_LOGE("Read event header fails");
                goto CleanUp;
            }

            u4Len += 2;
            pkt_len = (unsigned int)ucRxBuf[2];
            if ((u4Len + pkt_len) > sizeof(ucRxBuf)) {
                BTTOOL_LOGE("Read buffer overflow! packet len %d", u4Len + pkt_len);
                goto CleanUp;
            }

            if (bt_driver_rx_timeout(&ucRxBuf[3], pkt_len) < 0) {
                BTTOOL_LOGE("Read event param fails");
                goto CleanUp;
            }

            u4Len += pkt_len;

            /* Dump rx packet */
            /*BTTOOL_LOGI("read:\n");
            for (i = 0; i < u4Len; i++) {
                 BTTOOL_LOGI("%02x\n", ucRxBuf[i]);
            }*/
            bt_tool_dump_buffer("inq result", ucRxBuf, u4Len);

            if (ucRxBuf[1] == 0x0F) {
                /* Command status event */
                if (pkt_len != 0x04) {
                    BTTOOL_LOGE("Unexpected command status event len %d", pkt_len);
                    goto CleanUp;
                }

                if (ucRxBuf[3] != 0x00) {
                    BTTOOL_LOGE("Unexpected command status %02x", ucRxBuf[3]);
                    goto CleanUp;
                }
            }
            else if (ucRxBuf[1] == 0x01) {
                /* Inquiry complete event */
                if (pkt_len != 0x01) {
                    BTTOOL_LOGE("Unexpected inquiry complete event len %d", pkt_len);
                    goto CleanUp;
                }

                if (ucRxBuf[3] != 0x00) {
                    BTTOOL_LOGE("Unexpected inquiry complete status %02x", ucRxBuf[3]);
                    goto CleanUp;
                }

                BTTOOL_LOGW("---Inquiry completed---");
                inq_complete = TRUE;
            }
            else if (ucRxBuf[1] == 0x02 || ucRxBuf[1] == 0x22 || ucRxBuf[1] == 0x2F) {
                /* Inquiry result event */
                /*
                if (pkt_len != 0x0F) {
                    ERR("Unexpected inquiry result event len %d", pkt_len);
                    goto CleanUp;
                }
             */

                /* Retrieve BD addr */
                btaddr[0] = ucRxBuf[9];
                btaddr[1] = ucRxBuf[8];
                btaddr[2] = ucRxBuf[7];
                btaddr[3] = ucRxBuf[6];
                btaddr[4] = ucRxBuf[5];
                btaddr[5] = ucRxBuf[4];

                /* Inquiry result callback */
                memset(str, 0, sizeof(str));
                p_str = str;
                p_str += sprintf(p_str, "    %02x:%02x:%02x:%02x:%02x:%02x",
                    btaddr[0], btaddr[1], btaddr[2], btaddr[3], btaddr[4], btaddr[5]);

                if (ucRxBuf[1] == 0x22 || ucRxBuf[1] == 0x2F) {
                    char rssi = ~ucRxBuf[17] + 1;
                    p_str += sprintf(p_str, ", RSSI:%s%d", ucRxBuf[17] > 0x7F ? "-" : "",
                            ucRxBuf[17] > 0x7F ? rssi : ucRxBuf[17]);
                }
                if (ucRxBuf[1] == 0x2F) {
                    int i = 18;
                    while (ucRxBuf[i]) {
                        if (ucRxBuf[i + 1] == 8 || ucRxBuf[i + 1] == 9) {
                            char name[128] = {0};
                            memcpy(name, &ucRxBuf[i + 2], ucRxBuf[i] - 1);
                            name[ucRxBuf[i] - 1] = '\0';
                            p_str += sprintf(p_str, ", Name:%s", name);
                        }
                        i += (ucRxBuf[i] + 1);
                    }
                }
                p_str += sprintf(p_str, "\n");
                BTTOOL_LOGI("Device: %s", str);
            }
            else {
                /* simply ignore it? */
                BTTOOL_LOGE("Unexpected event %02x", ucRxBuf[1]);
            }
            break;

          default:
            BTTOOL_LOGE("Unexpected BT packet header %02x", ucHeader);
            goto CleanUp;
        }
    }
    return TRUE;
CleanUp:
    BTTOOL_LOGE("Inquiry complete failed");
    return FALSE;
}

BOOL HW_TEST_BT_Inquiry(void)
{
    UINT8 INQ_MODE[] = {0x01, 0x45, 0x0C, 0x01, 0x02};

    UINT8 ucAckEvent[7];
    /* Event expected */
    UINT8 ucEvent[] = {0x04, 0x0E, 0x04, 0x01, 0x45, 0x0C, 0x00};

    UINT8 HCI_INQUIRY[] =
        {0x01, 0x01, 0x04, 0x05, 0x33, 0x8B, 0x9E, 0x05, 0x0A};
    UINT32 i;

    BTTOOL_LOG_FUNC();

    if (!b_drv_open) {
        BTTOOL_LOGE("bt driver is not open!");
        return FALSE;
    }

    if (bt_driver_tx(INQ_MODE, sizeof(INQ_MODE)) < 0) {
        BTTOOL_LOGE("Send inquiry mode command fail");
        return FALSE;
    }
    bt_tool_dump_buffer("INQ_MODE", INQ_MODE, sizeof(INQ_MODE));

    /* Receive command complete event */
    if (bt_driver_rx_timeout(ucAckEvent, sizeof(ucAckEvent)) < 0) {
        BTTOOL_LOGE("Receive command complete event fail");
        return FALSE;
    }
    bt_tool_dump_buffer("RECV EVENT", ucAckEvent, sizeof(ucAckEvent));

    if (memcmp(ucAckEvent, ucEvent, sizeof(ucEvent))) {
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }

    if (bt_driver_tx(HCI_INQUIRY, sizeof(HCI_INQUIRY)) < 0) {
        BTTOOL_LOGE("Send inquiry command fail");
        return FALSE;
    }
    bt_tool_dump_buffer("HCI_INQUIRY", HCI_INQUIRY, sizeof(HCI_INQUIRY));

    return TRUE;
}

BOOL HW_TEST_BT_LE_Tx_start(BT_HW_TEST_CMD_T *test_cmd)
{
    UINT8 HCI_LE_TX_TEST[] = {0x01, 0x1E, 0x20, 0x03,
                              0x00, /* Tx channel */
                              0x25, /* Packet payload data length */
                              0x00};/* Tx payload pattern */
    UINT8 ucAckEvent[7];
    /* Event expected */
    UINT8 ucEvent[] = {0x04, 0x0E, 0x04, 0x01, 0x1E, 0x20, 0x00};
    UINT32 i;

    BTTOOL_LOG_FUNC();

    if (!b_drv_open) {
        BTTOOL_LOGE("bt driver is not open!");
        return FALSE;
    }

    /* Prepare LE Tx test command */
    HCI_LE_TX_TEST[4] = (UINT8)test_cmd->params.cmd_ble_tx.channel;
    HCI_LE_TX_TEST[6] = test_cmd->params.cmd_ble_tx.tx_pattern;

    if (bt_driver_tx(HCI_LE_TX_TEST, sizeof(HCI_LE_TX_TEST)) < 0) {
        BTTOOL_LOGE("Send LE Tx test command fail");
        return FALSE;
    }
    bt_tool_dump_buffer("HCI_LE_TX_TEST", HCI_LE_TX_TEST, sizeof(HCI_LE_TX_TEST));

    /* Receive command complete event */
    if (bt_driver_rx_timeout(ucAckEvent, sizeof(ucAckEvent)) < 0) {
        BTTOOL_LOGE("Receive command complete event fail");
        return FALSE;
    }
    bt_tool_dump_buffer("RECV EVENT", ucAckEvent, sizeof(ucAckEvent));

    if (memcmp(ucAckEvent, ucEvent, sizeof(ucEvent))) {
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }

    return TRUE;
}

BOOL HW_TEST_BT_LE_Tx_end(void)
{
    UINT8 HCI_LE_TEST_END[] = {0x01, 0x1F, 0x20, 0x00};
    UINT8 ucAckEvent[9];
    /* Event expected, for tx test, the last two bytes are 0x0000 */
    UINT8 ucEvent[] = {0x04, 0x0E, 0x06, 0x01, 0x1F, 0x20, 0x00, 0x00, 0x00};
    UINT32 i;

    BTTOOL_LOG_FUNC();

    if (!b_drv_open) {
        BTTOOL_LOGE("bt driver is not open!");
        return FALSE;
    }

    /* LE test end command */
    if (bt_driver_tx(HCI_LE_TEST_END, sizeof(HCI_LE_TEST_END)) < 0) {
        BTTOOL_LOGE("Send LE test end command fail");
        return FALSE;
    }
    bt_tool_dump_buffer("HCI_LE_TEST_END", HCI_LE_TEST_END, sizeof(HCI_LE_TEST_END));

    /* Receive command complete event */
    if (bt_driver_rx_timeout(ucAckEvent, sizeof(ucAckEvent)) < 0) {
        BTTOOL_LOGE("Receive command complete event fail");
        return FALSE;
    }
    bt_tool_dump_buffer("RECV EVENT", ucAckEvent, sizeof(ucAckEvent));

    if (memcmp(ucAckEvent, ucEvent, sizeof(ucEvent))) {
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }

    return TRUE;
}

BOOL HW_TEST_BT_LE_Rx_start(BT_HW_TEST_CMD_T *test_cmd)
{
    UINT8 HCI_LE_RX_TEST[] = {0x01, 0x1D, 0x20, 0x01,
                              0x00};/* Rx channel */
    UINT8 ucAckEvent[7];
    /* Event expected */
    UINT8 ucEvent[] = {0x04, 0x0E, 0x04, 0x01, 0x1D, 0x20, 0x00};
    UINT32 i;

    BTTOOL_LOG_FUNC();

    if (!b_drv_open) {
        BTTOOL_LOGE("bt driver is not open!");
        return FALSE;
    }

    /* Prepare LE Rx test command */
    HCI_LE_RX_TEST[4] = (UINT8)test_cmd->params.cmd_ble_rx.channel;

    if (bt_driver_tx(HCI_LE_RX_TEST, sizeof(HCI_LE_RX_TEST)) < 0) {
        BTTOOL_LOGE("Send LE Rx test command fail");
        return FALSE;
    }
    bt_tool_dump_buffer("HCI_LE_RX_TEST", HCI_LE_RX_TEST, sizeof(HCI_LE_RX_TEST));

    /* Receive command complete event */
    if (bt_driver_rx_timeout(ucAckEvent, sizeof(ucAckEvent)) < 0){
        BTTOOL_LOGE("Receive command complete event fail");
        return FALSE;
    }
    bt_tool_dump_buffer("RECV EVENT", ucAckEvent, sizeof(ucAckEvent));

    if (memcmp(ucAckEvent, ucEvent, sizeof(ucEvent))) {
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }

    return TRUE;
}

BOOL HW_TEST_BT_LE_Rx_end(UINT16 *pu2RxPktCount)
{
    UINT8 HCI_LE_TEST_END[] = {0x01, 0x1F, 0x20, 0x00};
    UINT8 ucAckEvent[9];
    /* Event expected, for rx test, the last two bytes are the total received packet count */
    UINT8 ucEvent[] = {0x04, 0x0E, 0x06, 0x01, 0x1F, 0x20, 0x00};
    UINT32 i;

    BTTOOL_LOG_FUNC();

    if (!b_drv_open) {
        BTTOOL_LOGE("bt driver is not open!");
        return FALSE;
    }

    /* LE test end command */
    if (bt_driver_tx(HCI_LE_TEST_END, sizeof(HCI_LE_TEST_END)) < 0) {
        BTTOOL_LOGE("Send LE test end command fail");
        return FALSE;
    }
    bt_tool_dump_buffer("HCI_LE_TEST_END", HCI_LE_TEST_END, sizeof(HCI_LE_TEST_END));

    /* Receive command complete event */
    if (bt_driver_rx_timeout(ucAckEvent, sizeof(ucAckEvent)) < 0) {
        BTTOOL_LOGE("Receive command complete event fail");
        return FALSE;
    }
    bt_tool_dump_buffer("RECV EVENT", ucAckEvent, sizeof(ucAckEvent));

    if (memcmp(ucAckEvent, ucEvent, sizeof(ucEvent))) {
        BTTOOL_LOGE("Receive unexpected event");
        return FALSE;
    }
    else {
        *pu2RxPktCount = *((UINT16*)&ucAckEvent[7]);
    }

    return TRUE;
}

BOOL HW_TEST_TX_POWER_OFFSET_op(int wr_flag, char *group_offset, char *group_flag)
{
    BTTOOL_LOGE("%s NOT SUPPORT");
    return FALSE;
}

