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
#ifndef __BOOTS_STRESS_H__
#define __BOOTS_STRESS_H__

// Print all CMD log in console
#define BOOTS_STRESS_SHOW_ALL_CMD 0
// Print all EVENT log in console
#define BOOTS_STRESS_SHOW_ALL_EVENT 0
// Measure the latency in 1:boots, 0:boots_srv
#define BOOTS_STRESS_MEASURE_IN_BOOTS 0
// The latency result includes :
//    1 : ACL_OUT + EVENT_IN(NOCP) + ACL_IN
//    0 : ACL_OUT + EVENT_IN(NOCP)
#define BOOTS_STRESS_MEASURE_LBT_TOTAL_LATENCY 0

// Maximum allowed latency (us)
#define BOOTS_STRESS_MAX_ALLOWED_LATENCY 50000
// Maximum records per second
#define BOOTS_STRESS_TIMESTAMP_RECORD_MAX_NUM 2048
// Maximum stress test packet size (Max Local Name Size)
#define BOOTS_STRESS_STRESS_TEST_MAX_PKT_SIZE 248
// Maximum loopback test packet size (Max ACL Size)
#define BOOTS_STRESS_LOOPBACK_TEST_MAX_PKT_SIZE 1021

enum{
    BOOTS_STRESS_THREAD_STATE_UNKNOWN,
    BOOTS_STRESS_THREAD_STATE_THREAD_RUNNING,
    BOOTS_STRESS_THREAD_STATE_THREAD_STOPPED,
};

enum{
    BOOTS_STRESS_TIMESTAMP_SEND_CMD_START,
    BOOTS_STRESS_TIMESTAMP_SEND_CMD_FINISH,
    BOOTS_STRESS_TIMESTAMP_RECEIVE_EVENT_FINISH,
};

extern void boots_stress_record_timestamp(uint8_t timestamp_type);
extern void boots_stress_init(void);
extern void boots_stress_deinit(void);
#endif // __BOOTS_STRESS_H__