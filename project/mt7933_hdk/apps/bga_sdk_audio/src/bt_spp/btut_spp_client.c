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

#include "FreeRTOS.h"
#include "semphr.h"

#include <string.h>

#include "btut_spp_main.h"
#include "btut_spp_client.h"

xSemaphoreHandle g_spp_client_mtx = NULL;

btut_spp_cntx_t g_spp_client_cntx;

static void _btut_spp_client_lock(void)
{
    if (g_spp_client_mtx)
        xSemaphoreTake(g_spp_client_mtx, portMAX_DELAY);
    else
        LOG_E(BTUT_SPP, "Pls check why no mutex!!");
}

static void _btut_spp_client_unlock(void)
{
    if (g_spp_client_mtx)
        xSemaphoreGive(g_spp_client_mtx);
    else
        LOG_E(BTUT_SPP, "Pls check why no mutex!!");
}

static void _btut_spp_client_reset_cntx(void)
{
    uint8_t *rx_buf = g_spp_client_cntx.p_rx_data_buf;
    btut_spp_ready_to_send_cb send_cb = g_spp_client_cntx.ready_to_send_cb;

    memset(&g_spp_client_cntx, 0, sizeof(g_spp_client_cntx));
    g_spp_client_cntx.p_rx_data_buf = rx_buf;
    g_spp_client_cntx.ready_to_send_cb = send_cb;
}

static bool _btut_spp_client_check_handle(uint32_t spp_handle)
{
    if (g_spp_client_cntx.spp_handle != spp_handle) {
        LOG_E(BTUT_SPP, "CLT: Not current supported handle, 0x%x (0x%x)",
              g_spp_client_cntx.spp_handle, spp_handle);
        return false;
    }
    return true;
}

static bool _btut_spp_client_is_connectable(void)
{
    if (g_spp_client_cntx.spp_handle == BT_SPP_INVALID_HANDLE)
        return true;
    return false;
}

static void _btut_spp_client_print_rx_data(uint8_t *p_rx_buf, uint32_t rx_len)
{
    // Flush out all the RX data
#define SPP_CLIENT_PRINT_SIZE 48 // (16x3 = 28) 3-bytes size on each hex char
    uint32_t i = 0, j;
    char pr_buf[SPP_CLIENT_PRINT_SIZE + 1];
    uint32_t pr_pos = 0;

    if (p_rx_buf == NULL || rx_len == 0)
        return;

    LOG_I(BTUT_SPP, "CLT: RX data start:(len = %d)", rx_len);
    for (i = 0, j = 1; i < rx_len; i++) {
        if (pr_pos < SPP_CLIENT_PRINT_SIZE) {
            pr_pos += snprintf(pr_buf + pr_pos, sizeof(pr_buf) - pr_pos, "%02x ", (unsigned int)p_rx_buf[i]);
        }

        if (pr_pos >= SPP_CLIENT_PRINT_SIZE || i == (rx_len - 1)) {
            LOG_I(BTUT_SPP, "CLT: [%d]%s", j, pr_buf);
            pr_pos = 0;
            j++;
        }
    }
    LOG_I(BTUT_SPP, "CLT: RX data end");
}

void btut_spp_client_event_handler(btut_spp_msg_t *p_msg)
{
    if (!p_msg)
        return;

    //LOG_I(BTUT_SPP,"client evt handler, 0x%x", p_msg->msg);
    _btut_spp_client_lock();

    switch (p_msg->msg) {
        case BT_SPP_CONNECT_CNF: { /*0x34000001*/
                LOG_I(BTUT_SPP, "CLT: CONNECT CNF (max len = %d)",
                      p_msg->param.conn_cnf.max_packet_length);

                if (!_btut_spp_client_check_handle(p_msg->param.conn_cnf.handle))
                    break;

                if (p_msg->status == BT_STATUS_SUCCESS) {
                    g_spp_client_cntx.is_connected = true;
                    g_spp_client_cntx.max_packet_len = p_msg->param.conn_cnf.max_packet_length;

                } else {
                    _btut_spp_client_reset_cntx();
                }

            }
            break;

        case BT_SPP_DISCONNECT_IND: { /*0x34000002*/
                LOG_I(BTUT_SPP, "CLT: DICONNECT IND");

                if (!_btut_spp_client_check_handle(p_msg->param.dis_ind.handle))
                    break;

                _btut_spp_client_reset_cntx();
            }
            break;

        case BT_SPP_READY_TO_SEND_IND: { /*0x34000004*/
                LOG_I(BTUT_SPP, "CLT: Ready2Send IND");
                if (!_btut_spp_client_check_handle(p_msg->param.send_ind.handle))
                    break;
                if (g_spp_client_cntx.ready_to_send_cb)
                    g_spp_client_cntx.ready_to_send_cb(p_msg->param.send_ind.handle);
                else
                    LOG_E(BTUT_SPP, "CLT: No ready to send callback!");
            }
            break;

        case BT_SPP_DATA_RECEIVED_IND: { /*0x34000003*/
                if (!_btut_spp_client_check_handle(p_msg->param.data_ind.handle))
                    break;
                uint8_t *p_rx_buf = g_spp_client_cntx.p_rx_data_buf;
                uint32_t rx_len = g_spp_client_cntx.rx_data_len;
                uint32_t read_offset, new_read_offset = g_spp_client_cntx.rx_read;

                if (p_rx_buf == NULL)
                    break;

                //print all rx data
                while (rx_len) {
                    read_offset = g_spp_client_cntx.rx_read;
                    if (read_offset >= BTUT_SPP_CLIENT_RX_DATA_BUF) {
                        LOG_E(BTUT_SPP, "CLT: reset offset is wrong (%d)", read_offset);
                        break;
                    }

                    if (rx_len > BTUT_SPP_CLIENT_RX_DATA_BUF - read_offset) {
                        rx_len = BTUT_SPP_CLIENT_RX_DATA_BUF - read_offset;
                        new_read_offset = 0;
                    } else {
                        new_read_offset += rx_len;
                        if (new_read_offset == BTUT_SPP_CLIENT_RX_DATA_BUF)
                            new_read_offset = 0;
                    }

                    //unlock before printing
                    _btut_spp_client_unlock();
                    _btut_spp_client_print_rx_data(p_rx_buf + read_offset, rx_len);
                    _btut_spp_client_lock();
                    g_spp_client_cntx.rx_data_len -= rx_len;
                    g_spp_client_cntx.rx_read = new_read_offset;
                    rx_len = g_spp_client_cntx.rx_data_len;
                }

            }
            break;

        default:
            LOG_W(BTUT_SPP, "CLT: unused msg 0x%0x", p_msg->msg);
            break;
    }

    _btut_spp_client_unlock();

}

uint16_t btut_spp_client_feed_rx_data(uint32_t handle, uint8_t *buf, uint16_t len)
{
    uint16_t cp_len = 0; //processed len
    uint8_t *p_rx_buf = NULL;
    uint32_t used_size = 0, free_size = 0;
    uint32_t write_offset, tail_size;

    _btut_spp_client_lock();

    if (!_btut_spp_client_check_handle(handle))
        goto SPP_CLT_FEED_END;

    p_rx_buf = g_spp_client_cntx.p_rx_data_buf;
    write_offset = g_spp_client_cntx.rx_write;
    used_size = g_spp_client_cntx.rx_data_len;

    if (p_rx_buf == NULL) {
        LOG_E(BTUT_SPP, "CLT: Why rx buf null?");
        goto SPP_CLT_FEED_END;
    }

    free_size = BTUT_SPP_CLIENT_RX_DATA_BUF - used_size;
    if (len > free_size) {
        LOG_W(BTUT_SPP, "CLT: no free rx space (%d/%d)", len, free_size);
        goto SPP_CLT_FEED_END;
    }

    tail_size = BTUT_SPP_CLIENT_RX_DATA_BUF - write_offset;
    if (tail_size >= len) {
        memcpy(p_rx_buf + write_offset, buf, len);
        g_spp_client_cntx.rx_write += len;
    } else {
        memcpy(p_rx_buf + write_offset, buf, tail_size);
        memcpy(p_rx_buf, buf + tail_size, len - tail_size);
        g_spp_client_cntx.rx_write = len - tail_size;
    }
    g_spp_client_cntx.rx_data_len += len;
    cp_len = len;

SPP_CLT_FEED_END:
    _btut_spp_client_unlock();
    return cp_len;
}


bt_status_t btut_spp_client_send_data(uint32_t handle, uint8_t *buf, uint16_t len)
{
    bt_status_t status = BT_STATUS_FAIL;

    _btut_spp_client_lock();

    if (!_btut_spp_client_check_handle(handle))
        goto SPP_CLT_SEND_END;

    if (len > g_spp_client_cntx.max_packet_len) {
        LOG_W(BTUT_SPP, "CLT: over max packet len %d (in %d)",
              g_spp_client_cntx.max_packet_len, len);
        goto SPP_CLT_SEND_END;
    }

    status = bt_spp_send(handle, buf, len);

SPP_CLT_SEND_END:
    _btut_spp_client_unlock();
    return status;
}


void btut_spp_client_connect_req(const bt_bd_addr_t *remote_addr, const uint8_t *spp_uuid)
{
    uint32_t handle = BT_SPP_INVALID_HANDLE;
    bt_status_t result;

    _btut_spp_client_lock();

    if (!_btut_spp_client_is_connectable()) {
        _btut_spp_client_unlock();
        return;
    }
    _btut_spp_client_unlock();

    result = bt_spp_connect(&handle, remote_addr, spp_uuid);
    if (result != BT_STATUS_SUCCESS) {
        LOG_E(BTUT_SPP, "conn req FAIL: result:0x%x", result);
    } else {
        _btut_spp_client_lock();
        g_spp_client_cntx.spp_handle = handle;
        g_spp_client_cntx.is_initiator = true;
        _btut_spp_client_unlock();
        LOG_E(BTUT_SPP, "conn req success. (hdl = 0x%x)", handle);
    }
}

uint16_t btut_spp_client_get_max_packet_len(uint32_t handle)
{
    uint16_t len;

    _btut_spp_client_lock();
    len = g_spp_client_cntx.max_packet_len;
    _btut_spp_client_unlock();
    return len;
}


bool btut_spp_is_client(uint32_t handle)
{
    bool ret;
    _btut_spp_client_lock();
    if (g_spp_client_cntx.is_initiator &&
        g_spp_client_cntx.spp_handle == handle)
        ret = true;
    else
        ret = false;
    _btut_spp_client_unlock();
    return ret;
}

void btut_spp_client_show_info(void)
{
    _btut_spp_client_lock();
    LOG_I(BTUT_SPP, "CLT:hdl = 0x%x, max_pack_len =%d", g_spp_client_cntx.spp_handle, g_spp_client_cntx.max_packet_len);
    LOG_I(BTUT_SPP, "CLT:connected = %d, rx_buf_len = %d", g_spp_client_cntx.is_connected, g_spp_client_cntx.rx_data_len);
    _btut_spp_client_unlock();
}

void btut_spp_client_init(btut_spp_ready_to_send_cb send_cb)
{
    memset(&g_spp_client_cntx, 0, sizeof(g_spp_client_cntx));
    g_spp_client_cntx.ready_to_send_cb = send_cb;

    g_spp_client_cntx.p_rx_data_buf = pvPortMalloc(BTUT_SPP_CLIENT_RX_DATA_BUF);
    if (g_spp_client_cntx.p_rx_data_buf == NULL) {
        LOG_E(BTUT_SPP, "CLT:create rx buf fail");
        goto SPP_CLT_INIT_FAIL;
    }

    if (g_spp_client_mtx == NULL) {
        if ((g_spp_client_mtx = xSemaphoreCreateMutex()) == NULL) {
            LOG_E(BTUT_SPP, "CLT:create mutex fail");
            goto SPP_CLT_INIT_FAIL;
        }
    }

    return;

SPP_CLT_INIT_FAIL:
    if (g_spp_client_cntx.p_rx_data_buf) {
        vPortFree(g_spp_client_cntx.p_rx_data_buf);
        g_spp_client_cntx.p_rx_data_buf = NULL;
    }

}

void btut_spp_client_deinit(void)
{

    if (g_spp_client_cntx.p_rx_data_buf) {
        vPortFree(g_spp_client_cntx.p_rx_data_buf);
        g_spp_client_cntx.p_rx_data_buf = NULL;
    }

    memset(&g_spp_client_cntx, 0, sizeof(g_spp_client_cntx));

    if (g_spp_client_mtx) {
        vSemaphoreDelete(g_spp_client_mtx);
        g_spp_client_mtx = NULL;
    }

}

