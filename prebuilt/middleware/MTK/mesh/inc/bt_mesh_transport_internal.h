/* Copyright Statement:
 *
 * (C) 2017  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
 */

#ifndef __MESH_TRANSPORT_INTERNAL_H__
#define __MESH_TRANSPORT_INTERNAL_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "bt_mesh_network_internal.h"

#define MESH_TRANSLAYER_CTLMSG_OPCODE_SEG_ACK     0x00
#define MESH_TRANSLAYER_CTLMSG_OPCODE_FRIEND_POLL     0x01
#define MESH_TRANSLAYER_CTLMSG_OPCODE_FRIEND_UPDATE     0x02
#define MESH_TRANSLAYER_CTLMSG_OPCODE_FRIEND_REQUEST     0x03
#define MESH_TRANSLAYER_CTLMSG_OPCODE_FRIEND_OFFER     0x04
#define MESH_TRANSLAYER_CTLMSG_OPCODE_FRIEND_CLEAR     0x05
#define MESH_TRANSLAYER_CTLMSG_OPCODE_FRIEND_CLEAR_CFM     0x06
#define MESH_TRANSLAYER_CTLMSG_OPCODE_FRIEND_SUBLIST_ADD     0x07
#define MESH_TRANSLAYER_CTLMSG_OPCODE_FRIEND_SUBLIST_REMOVE     0x08
#define MESH_TRANSLAYER_CTLMSG_OPCODE_FRIEND_SUBLIST_CFM     0x09
#define MESH_TRANSLAYER_CTLMSG_OPCODE_HEARTBEAT     0x0A

/* Spec 3.5.3.3 Segmentation behavior: Each Lower Transport PDU for an Upper
 * Transport PDU shall be transmitted at least two times unless acknowledged
 * earlier.
 */
#define MESH_DEFAULT_RETRANSMISSION 3

/* Transport Layer PDU Format */
#define TRANSPORT_PDU_SEG_MASK   0x80
#define TRANSPORT_PDU_SEG_SHIFT    7
#define TRANSPORT_PDU_AKF_MASK   0x40
#define TRANSPORT_PDU_AKF_SHIFT    6
#define TRANSPORT_PDU_AID_MASK    0x3F

#define TRANSPORT_PDU_OPCODE_MASK    0x7F
#define TRANSPORT_PDU_OBO_MASK  0x80
#define TRANSPORT_PDU_OBO_SHIFT  7

#define TRANSPORT_PDU_SZMIC_MASK   0x80
#define TRANSPORT_PDU_SZMIC_SHIFT    7

#define TRANSPORT_PDU_SEQ_ZERO_MASK_1   0x7F
#define TRANSPORT_PDU_SEQ_ZERO_SHIFT_1  0
#define TRANSPORT_PDU_SEQ_ZERO_MASK_2   0xFC
#define TRANSPORT_PDU_SEQ_ZERO_SHIFT_2  2
#define TRANSPORT_PDU_SEG_O_MASK_1  0x03
#define TRANSPORT_PDU_SEG_O_SHIFT_1 0
#define TRANSPORT_PDU_SEG_O_MASK_2  0xE0
#define TRANSPORT_PDU_SEG_O_SHIFT_2 5
#define TRANSPORT_PDU_SEG_N_MASK    0x1F
#define TRANSPORT_PDU_SEG_N_SHIFT   0


#define TRANSPORT_PDU_SEQ_NUMBER_LEN    3
#define TRANSPORT_PDU_SRC_ADDR_LEN    2
#define TRANSPORT_PDU_DST_ADDR_LEN    2
#define TRANSPORT_PDU_TRANSPORT_PDU_LEN    16
#define TRANSPORT_PDU_MAX_UNSEGMENTED_CTRL_SIZE   11
#define TRANSPORT_PDU_MAX_UNSEGMENTED_ACCESS_SIZE 15
#define TRANSPORT_PDU_MAX_SEGMENTED_CTRL_SIZE 8
#define TRANSPORT_PDU_MAX_SEGMENTED_ACCESS_SIZE 12

#define TRANSPORT_PDU_MAX_CONTROL_PDU_SIZE   256
#define TRANSPORT_PDU_MAX_ACCESS_PDU_SIZE   380 /* with 4-byte MIC*/

typedef enum {
    LOWER_TRANSPORT_PDU_UNSEG_ACCESS_MSG = 0,
    LOWER_TRANSPORT_PDU_SEG_ACCESS_MSG,
    LOWER_TRANSPORT_PDU_UNSEG_CTRL_MSG,
    LOWER_TRANSPORT_PDU_SEGMENT_ACK_MSG,
    LOWER_TRANSPORT_PDU_SEG_CTRL_MSG,
} lower_transport_pdu_type;

typedef struct {
    uint8_t seg : 1;  /**< shall be 0 = unsegmented message */
    uint8_t akf : 1;  /**< application key flag */
    uint8_t aid : 6;  /**< application key identifier */
    uint8_t sdu_len;
    uint8_t *transport_sdu;   /**< 5~15 octets */
} __attribute__((packed)) mesh_transport_unsegmented_access_msg;

typedef struct {
    uint32_t seg : 1;  /**< shall be 1 = segmented message */
    uint32_t akf : 1;  /**< application key flag */
    uint32_t aid : 6;  /**< application key identifier */
    uint32_t szmic : 1;   /**< size of transMIC, 0: 32bits, 1: 64 bits */
    uint32_t seqZero : 13; /**< leaast significant bits of SeqAuth */
    uint32_t segO : 5; /**< segment offset number */
    uint32_t segN : 5; /**< last segment number */
    uint16_t upper_pdu_len;
    uint8_t *upper_pdu; /**< segment m of the upper transport access PDU, 1~12 octets */
} __attribute__((packed)) mesh_transport_segmented_access_msg;

typedef struct {
    /*** keep this order, must fit mesh_transport_control_msg (Begin) ****/
    uint8_t seg : 1;  /**< shall be 0 = unsegmented message */
    uint8_t opcode : 7;   /**< 0x00: segment acknowledgement, 0x01 to 0x3F: opcode of the transport control msg */
    uint16_t parameters_len;
    uint8_t *parameters;  /**< parameters for the transport control message, 0~11 octets */
    /*** keep this order, must fit mesh_transport_control_msg (End) ****/
} __attribute__((packed)) mesh_transport_unsegmented_control_msg;

typedef struct {
    uint8_t seg : 1;  /**< shall be 0 = unsegmented message */
    uint8_t opcode : 7;   /**< shell be 0x00 = Segment Acknowledgement Message */
    uint16_t obo : 1;  /**< friend on behalf of a Low Power node */
    uint16_t seqZero : 13; /**< SeqZero for the upper transport PDU */
    uint16_t rfu : 2;  /**< reserved for future use */
    uint32_t block_ack;    /**< block acknowledgement for segments */
} __attribute__((packed)) mesh_transport_segment_acknowledgement_msg;

typedef struct {
    uint32_t seg : 1;  /**< shall be 1 = segmented message */
    uint32_t opcode : 7;   /**< 0x00: rfu, 0x01 to 0x3F: opcode of the transport control msg */
    uint32_t rfu : 1;  /**< reserved for future use */
    uint32_t seqZero : 13; /**< leaast significant bits of SeqAuth */
    uint32_t segO : 5; /**< segment offset number */
    uint32_t segN : 5; /**< last segment number */
    uint8_t *upper_pdu; /**< segment m of the upper transport access PDU, 1~12 octets */
    uint16_t upper_pdu_len;
} __attribute__((packed)) mesh_transport_segmented_control_msg;

/* Transport Layer PDU parameter. */
typedef struct {
    lower_transport_pdu_type type;
    void *sublist; /**< a double-linked list to store subscripted target address, if NULL means no subscription in this node */
    union {
        mesh_transport_unsegmented_access_msg unseg_access_msg;
        mesh_transport_segmented_access_msg seg_access_msg;
        mesh_transport_unsegmented_control_msg unseg_ctrl_msg;
        mesh_transport_segment_acknowledgement_msg segment_ack_msg;
        mesh_transport_segmented_control_msg seg_ctrl_msg;
    } pdu;
} mesh_lower_transport_pdu;

typedef struct {
    /*** keep this order (Begin) ****/
    uint8_t rvd : 1;        /**< reserved bit */
    uint8_t opcode : 7;     /**< operation code */
    uint16_t buflen;        /**< parameter length */
    uint8_t *buf;           /**< parameters */
    /*** keep this order (End) ****/
} __attribute__((packed)) mesh_transport_control_msg;

void mesh_transport_dump(void);
void mesh_transport_deinit( void );
void mesh_transport_init( void );
void mesh_transport_prepare_sleep(void);

void Mesh_Upper_Transport_ComposeAccessPdu(uint8_t *key, uint8_t akfaid, uint8_t ttl,
        uint16_t src_len, const uint8_t *src, uint16_t src_addr, uint16_t dst_addr, const uint8_t *labelUUID,
        uint16_t netkeyIdx, uint8_t credential_flag, uint8_t from_bearer);


void Mesh_Lower_Transport_ComposeControlPdu(uint16_t buf_len, const uint8_t *buf, uint8_t opcode,
        uint8_t ttl, uint16_t src_addr, uint16_t dst_addr, uint16_t netkeyIdx);

void Mesh_Lower_Transport_receiveFromNetwork(
    mesh_network_pdu_params *netpdu, uint16_t netkeyIdx, bool isFromFriend, bool isFromGatt);

#endif // __MESH_TRANSPORT_INTERNAL_H__

