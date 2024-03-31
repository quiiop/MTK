/* Copyright Statement:
 *
 * (C) 2022  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#ifndef __BTUT_SPP_MAIN_H__
#define __BTUT_SPP_MAIN_H__

#include "bt_system.h"
#include "bt_spp.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

#define BTUT_SPP_PERF_TEST_SUPPORT (TRUE)

#define BTUT_SPP_SRV_TX_DATA_BUF 1024
#define BTUT_SPP_SRV_RX_DATA_BUF 2048

#define BTUT_SPP_CLIENT_TX_DATA_BUF 1024
#define BTUT_SPP_CLIENT_RX_DATA_BUF 2048

#define BT_SPP_STANDARD_UUID    0x00,0x00,0x11,0x01,0x00,0x00,0x10,0x00,\
                                0x80,0x00,0x00,0x80,0x5F,0x9B,0x34,0xFB


#define BTUT_SPP_PERF_TASK_EVENT_TX_START      (0x1 << 0)
#define BTUT_SPP_PERF_TASK_EVENT_TX_CONTINUE   (0x1 << 1)
#define BTUT_SPP_PERF_TASK_EVENT_RX_ACK        (0x1 << 2)
#define BTUT_SPP_PERF_TASK_EVENT_ALL           0x0F

typedef enum {
    BTUT_SPP_ROLE_NONE = 0,
    BTUT_SPP_ROLE_CLIENT,
    BTUT_SPP_ROLE_SERVER,
} btut_spp_role_t;


typedef void (*btut_spp_ready_to_send_cb)(uint32_t handle);
typedef bt_status_t (*btut_spp_tx_fptr)(uint32_t handle, uint8_t *buf, uint16_t len);

/* Basic context struct for server and client */
typedef struct {
    uint32_t spp_handle;
    uint16_t max_packet_len;
    bool is_connected;
    bool is_initiator;
    /* SPP APP TX */
    //uint8_t *p_tx_data_buf;
    //uint32_t tx_data_len;
    //bool wait_tx_ready;
    /* SPP APP RX */
    uint8_t *p_rx_data_buf;
    uint32_t rx_read;  // the offset of rx buf read
    uint32_t rx_write; // the offset of rx buf write
    uint32_t rx_data_len;
    btut_spp_ready_to_send_cb  ready_to_send_cb;
} btut_spp_cntx_t;

typedef union {
    bt_spp_connect_ind_t conn_ind;
    bt_spp_connect_cnf_t conn_cnf;
    bt_spp_disconnect_ind_t dis_ind;
    bt_spp_data_received_ind_t data_ind;
    bt_spp_ready_to_send_ind_t send_ind;
} btut_spp_msg_param_t;

typedef struct {
    bt_msg_type_t msg;
    bt_status_t status;
    btut_spp_msg_param_t param;
} btut_spp_msg_t;


void btut_spp_init(void);

void btut_spp_deinit(void);

bt_status_t btut_spp_io_callback(void *input, void *output);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifndef __BTUT_SPP_MAIN_H__ */
