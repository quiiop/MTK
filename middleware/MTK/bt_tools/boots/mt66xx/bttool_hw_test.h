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
 * MediaTek Inc. (C) 2010. All rights reserved.
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

#ifndef __BTTOOL_HW_TEST_H_
#define __BTTOOL_HW_TEST_H_

#include "autobt.h"

int HW_TEST_BT_init(void);
void HW_TEST_BT_deinit(void);
BOOL HW_TEST_BT_reset(void);
BOOL HW_TEST_BT_TxOnlyTest_start(BT_HW_TEST_CMD_T *test_cmd);
BOOL HW_TEST_BT_TxOnlyTest_end(void);
BOOL HW_TEST_BT_NonSignalRx_start(BT_HW_TEST_CMD_T *test_cmd);
BOOL HW_TEST_BT_NonSignalRx_end(UINT32 *pu4RxPktCount, float *pftPktErrRate,
                                            UINT32 *pu4RxByteCount, float *pftBitErrRate);
BOOL HW_TEST_BT_TestMode_Rx_loop(void);
BOOL HW_TEST_BT_TestMode_enter(BT_HW_TEST_CMD_T *test_cmd);
BOOL HW_TEST_BT_TestMode_exit(void);
BOOL HW_TEST_BT_parse_inq_resp(void);
BOOL HW_TEST_BT_Inquiry(void);
BOOL HW_TEST_BT_LE_Tx_start(BT_HW_TEST_CMD_T *test_cmd);
BOOL HW_TEST_BT_LE_Tx_end(void);
BOOL HW_TEST_BT_LE_Rx_start(BT_HW_TEST_CMD_T *test_cmd);
BOOL HW_TEST_BT_LE_Rx_end(UINT16 *pu2RxPktCount);
BOOL HW_TEST_TX_POWER_OFFSET_op(int wr_flag, char *group_offset, char *group_flag);

#endif

