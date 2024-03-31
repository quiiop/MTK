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

#ifndef __BT_MESH_LPN_H__
#define __BT_MESH_LPN_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
 *   @addtogroup BluetoothMesh Mesh
 *   @{
 *   @addtogroup BluetoothMeshFeatures Features
 *   @{
        This section introduces the Low Power feature APIs. It shows how to use the APIs to establish friendship, terminate friendship, manage subscription list in Friend node, change resend poll message times, and enter Low Power mode.

        @section bt_mesh_lpn_api_usage How to create a Low Power node
        - The application calls functions #bt_mesh_model_set_composition_data_header(), #bt_mesh_lpn_request_friend(), #bt_mesh_lpn_establish_friendship(), and #bt_mesh_lpn_enable_low_power_mode().
        - First, please refer to \ref bt_mesh_model_api_usage to create a mesh device, and add #BT_MESH_FEATURE_LPN in feature field in composition data header as Low Power feature supported.
        - Second, please refer to \ref bt_mesh_api_usage to start Mesh module.
        - When become a node of the mesh network, it can start to request friend by using #bt_mesh_lpn_request_friend().
        - The event #BT_MESH_EVT_LPN_FRIEND_OFFER may be received many times, if potential Friend nodes offer friendships. Select a Friend from these events base on the information in #bt_mesh_evt_lpn_friend_offer_t, and send #bt_mesh_lpn_establish_friendship() within 1 second after the reception of the event.
        - If no event received or no acceptable Friend node, request friend procedure can be restarted. The time interval between two request friend procedures shall be greater than 1.1 seconds.
        - After friendship is established, the Low Power messaging operation will be executed by Mesh Low Power node module to receive stored messages and security updates from the Friend node.
        - When each messaging operation is completed, the node can enter Mesh Low Power mode by using #bt_mesh_lpn_enable_low_power_mode(). In this mode, Mesh function will be disabled, and will be enabled again before next messaging operation by Mesh Low Power Node module.
        - The function #bt_mesh_lpn_disable_low_power_mode() is used to disable Low Power mode and enable Mesh function.
        - If no response has been received from Friend node during messaging operation, the poll message may be resent according the setting of polltimeout. The resend times can be adjusted by using #bt_mesh_lpn_set_poll_resend_times(). The time interval between two poll messages will be different after resend times adjustment.
        - The subscription list in Friend node can be managed by using #bt_mesh_lpn_add_subscription_list() and #bt_mesh_lpn_remove_subscription_list(). The transactionNumber value shall start with 0, and increase after each operation. When each management operation is completed, an event @ref BT_MESH_EVT_LPN_FRIEND_SUBSCRIPTION_LIST_CONFRIM is received.
        - The existing friendship can be terminated by using #bt_mesh_lpn_terminate_friendship().
        - The result of friendship establishment and the status of friendship can be get through the event @ref BT_MESH_EVT_FRIENDSHIP_STATUS.

        - Sample code:
            @code
            static void create_mesh_device(void)
            {
                uint8_t composition_data_header[10] =
                {
                    0x94, 0x00, // cid
                    0x1A, 0x00, // pid
                    0x01, 0x00, // vid
                    0x08, 0x00, // crpl
                    BT_MESH_FEATURE_LPN, 0x00, // features
                };

                ...

                bt_mesh_model_set_composition_data_header(10, composition_data_header);

                ...
            }

            static bt_mesh_status_t friend_request(void)
            {
                uint8_t criteria = 0;
                uint8_t recv_delay = 0;
                uint32_t poll_timeout = 0;
                uint32_t poll_interval = 0;

                // criteria
                criteria = ((BT_MESH_LPN_CRITERIA_RSSI_FACTOR_2_0 << 5) |
                            (BT_MESH_LPN_CRITERIA_RECEIVE_WINDOW_FACTOR_1_5 << 3) |
                            (BT_MESH_LPN_CRITERIA_MIN_QUEUE_SIZE_LOG_2));

                // Receive Delay value: 0x0A ~ 0xFF, unit: 1 ms
                recv_delay = 0xA0;

                // poll_interval = receive_delay + receive_window(0xFF) + network_delay(0xD2) + next_poll_delay
                poll_interval = (recv_delay + 0xFF + 0xD2 + 0x1388);    // unit: 1 ms

                // poll_timeout = poll_interval*(2 + resend_times)
                poll_timeout = (poll_interval*(2+3));    // unit: 1 ms

                return bt_mesh_lpn_request_friend(criteria, recv_delay, poll_timeout, BT_MESH_GLOBAL_PRIMARY_NETWORK_KEY_INDEX);
            }

            static void bt_event(bt_evt_t *evt)
            {

				bt_mesh_event_t *mesh_event = (bt_mesh_event_t*)evt;
                ...
                switch(evt->evt_id) {
                    ...

                    case BT_MESH_EVT_FRIENDSHIP_STATUS: {
                        bt_mesh_evt_friendship_status_t *p = &mesh_event->evt.mesh_evt.mesh.friendship_status;
                        printf("BT_MESH_EVT_FRIENDSHIP_STATUS\n");
                        switch (p->status) {
                            case BT_MESH_FRIENDSHIP_TERMINATED:
                                printf("FRIENDSHIP_TERMINATED (Friend:%04x)\n", p->address);
                                break;
                            case BT_MESH_FRIENDSHIP_ESTABLISHED:
                                printf("FRIENDSHIP_ESTABLISHED (Friend:%04x)\n", p->address);
                                g_friendship_status = LPN_FRIENDSHIP_ESTABLISHED;
                                break;
                            case BT_MESH_FRIENDSHIP_ESTABLISH_FAILED:
                                printf("FRIENDSHIP_ESTABLISH_FAILED (Friend:%04x)\n", p->address);
                                break;
                            case BT_MESH_FRIENDSHIP_REQUEST_FRIEND_TIMEOUT:
                                printf("REQUEST_FRIEND_TIMEOUT\n");
                                break;
                            case BT_MESH_FRIENDSHIP_SELECT_FRIEND_TIMEOUT:
                                printf("SELECT_FRIEND_TIMEOUT");
                                break;
                        }
                        break;
                    }
                    case BT_MESH_EVT_LPN_FRIEND_OFFER: {
                        bt_mesh_evt_lpn_friend_offer_t *p = &mesh_event->evt.mesh_evt.mesh.lpn_friend_offer;
                        printf("BT_MESH_EVT_LPN_FRIEND_OFFER\n");
                        if (g_friendship_status == LPN_FRIENDSHIP_NONE) {
                            printf("addr:%04x receiveWindow:%x queueSize:%x\n", p->address, p->receive_window, p->queue_size);
                            printf("subscriptionListSize:%x rssi:%d lpn_counter:%04x\n", p->subscription_list_size, p->rssi, p->friend_counter);

                            // establish friendship
                            bt_mesh_lpn_establish_friendship(p->address, BT_MESH_GLOBAL_PRIMARY_NETWORK_KEY_INDEX);

                        } else {
                            printf("BT_MESH_EVT_LPN_FRIEND_OFFER ignore! \n");
                        }
                        break;
                    }
                    case BT_MESH_EVT_LPN_FRIEND_SUBSCRIPTION_LIST_CONFRIM: {
                        bt_mesh_evt_lpn_subscription_list_confirm_t *p = &mesh_event->evt.mesh_evt.mesh.lpn_subscription_list_confirm;
                        printf("BT_MESH_EVT_LPN_FRIEND_SUBSCRIPTION_LIST_CONFRIM\n");
                        printf("transaction_number:%x\n", p->transaction_number);
                        break;
                    }
                }
            }

            void main()
            {
                ...

                while(1) {
                    if (!bt_mesh_access_dispatch_rx_message()) {
                        return;
                    }

                    if (g_friendship_status != LPN_FRIENDSHIP_ESTABLISHED) {
                        return;
                    }

                    // mesh function enter low power mode
                    if (BT_MESH_SUCCESS == bt_mesh_lpn_enable_low_power_mode()) {

                        // prepare to enter sleep mode
                        ...
                    }
                };
            }
            @endcode
*/

/*!
    @name RSSI factor value
    @brief The value of RSSIFactor field which is used in the criteria parameter in #bt_mesh_lpn_request_friend().
    @note Please refer to Mesh Profile Specification v1.0 Section 3.6.5.3 for the information about the RSSIFactor field.
    @{
*/
#define BT_MESH_LPN_CRITERIA_RSSI_FACTOR_1_0 0x00  /**< Indicates the RSSIFactor value is 1 */
#define BT_MESH_LPN_CRITERIA_RSSI_FACTOR_1_5 0x01  /**< Indicates the RSSIFactor value is 1.5 */
#define BT_MESH_LPN_CRITERIA_RSSI_FACTOR_2_0 0x02  /**< Indicates the RSSIFactor value is 2 */
#define BT_MESH_LPN_CRITERIA_RSSI_FACTOR_2_5 0x03  /**< Indicates the RSSIFactor value is 2.5 */
/*!  @} */

/*!
    @name Receive window factor value
    @brief The value of ReceiveWindowFactor field which is used in the criteria parameter in #bt_mesh_lpn_request_friend().
    @note Please refer to Mesh Profile Specification v1.0 Section 3.6.5.3 for the information about the ReceiveWindowFactor field.
    @{
 */
#define BT_MESH_LPN_CRITERIA_RECEIVE_WINDOW_FACTOR_1_0 0x00    /**< Indicates the ReceiveWindowFactor value is 1 */
#define BT_MESH_LPN_CRITERIA_RECEIVE_WINDOW_FACTOR_1_5 0x01    /**< Indicates the ReceiveWindowFactor value is 1.5 */
#define BT_MESH_LPN_CRITERIA_RECEIVE_WINDOW_FACTOR_2_0 0x02    /**< Indicates the ReceiveWindowFactor value is 2 */
#define BT_MESH_LPN_CRITERIA_RECEIVE_WINDOW_FACTOR_2_5 0x03    /**< Indicates the ReceiveWindowFactor value is 2.5 */
/*!  @} */

/*!
    @name Minimum queue size log value
    @brief The value of MinQueueSizeLog field which is used in the criteria parameter in #bt_mesh_lpn_request_friend().
    @note Please refer to Mesh Profile Specification v1.0 Section 3.6.5.3 for the information about the MinQueueSizeLog field.
    @{
 */
#define BT_MESH_LPN_CRITERIA_MIN_QUEUE_SIZE_LOG_2      0x01    /**< Indicates the minimum queue size is 2 */
#define BT_MESH_LPN_CRITERIA_MIN_QUEUE_SIZE_LOG_4      0x02    /**< Indicates the minimum queue size is 4 */
#define BT_MESH_LPN_CRITERIA_MIN_QUEUE_SIZE_LOG_8      0x03    /**< Indicates the minimum queue size is 8 */
#define BT_MESH_LPN_CRITERIA_MIN_QUEUE_SIZE_LOG_16     0x04    /**< Indicates the minimum queue size is 16 */
#define BT_MESH_LPN_CRITERIA_MIN_QUEUE_SIZE_LOG_32     0x05    /**< Indicates the minimum queue size is 32 */
#define BT_MESH_LPN_CRITERIA_MIN_QUEUE_SIZE_LOG_64     0x06    /**< Indicates the minimum queue size is 64 */
#define BT_MESH_LPN_CRITERIA_MIN_QUEUE_SIZE_LOG_128    0x07    /**< Indicates the minimum queue size is 128 */
/*!  @} */



/** @brief Low power status. */
typedef enum {
    BT_MESH_LOW_POWER_STATUS_ENABLED,     /**< Low power is enabled. */
    BT_MESH_LOW_POWER_STATUS_DISABLED,    /**< Low power is disabled. */
    BT_MESH_LOW_POWER_STATUS_UNLOCKED,    /**< Low power is unlocked, need to enable. */
} bt_mesh_low_power_status_t;

/*!
    @brief Start a friendship request.
    @param[in] criteria is the criteria that a Friend node should support in order to participate in friendship negotiation.
    @param[in] receive_delay is the time between Low Power node sending a request and listening for a response. The valid range is 10 to 255. The unit is 1 millisecond.
    @param[in] poll_timeout is the initial value of the PollTimeout timer set by the Low Power node. The valid range is 1000 to 345599900. The unit is 1 millisecond.
    @param[in] netkey_index is the network key index used for this request.
    @return
    #BT_MESH_SUCCESS, the request is sent successfully.\n
    #BT_MESH_ERROR_INVALID_STATE, the friendship establishment is completed or in process, or the Friend feature is in use.\n
    #BT_MESH_ERROR_INVALID_PARAM, invaild parameter for request friend.\n
    #BT_MESH_ERROR_INVALID_ROLE, the Low Power feature is not supported.
    @note
    - When a Friend node is able to offer a friendship, a event #BT_MESH_EVT_LPN_FRIEND_OFFER is received.\n
    - criteria should be composed in this format:\n
    Field | Size (bits) | Notes
    ------|---------------|-------
    RFU | 1 | Reserved for Future Use
    RSSIFactor   | 2 | The contribution of the RSSI measured by the Friend node used in Friend Offer Delay calculations
    ReceiveWindowFactor | 2 | The contribution of the supported Receive Window used in Friend Offer Delay calculations
    MinQueueSizeLog | 3 | The minimum number of messages that the Friend node can store in its Friend Queue

    - poll_timeout should be computed with the formula:\n
    poll_timeout = poll_interval * (2 + resend_times) \n
    where:\n
    poll_interval = receive_delay + receive_window + network_delay + next_poll_delay\n
    The number 2 means current and next messaging operation\n
    Name | Notes
    -----|--------
    poll_interval | The time between two consecutive poll messages sending by Low Power node.\n
    resend_times | The poll messages resend times if Low Power node dose not receive a response within the Receive Window from Friend node.\n
    receive_window | The Receive Window value received from Friend node. The value can be considered as 255 ms.\n
    network_delay | The time between Low Power node sending the message and Friend node receiving this message. The value can be considered as 210 ms.\n
    next_poll_delay | The time between Low Power node receiveing message from Friend node and sending next poll message to Friend node.\n

*/
bt_mesh_status_t bt_mesh_lpn_request_friend(
    uint8_t criteria,
    uint8_t receive_delay,
    uint32_t poll_timeout,
    uint16_t netkey_index);

/*!
    @brief Establish a friendship with the specified node.
    @param[in] friend_addr is the unicast address of the Friend node.
    @param[in] netkey_index is the network key index used for this friendship.
    @return
    #BT_MESH_SUCCESS, the request is sent successfully.\n
    #BT_MESH_ERROR_INVALID_STATE, the friendship establishment is completed or in process, or the Friend feature is in use.\n
    #BT_MESH_ERROR_INVALID_PARAM, no valid friend for target address.\n
    #BT_MESH_ERROR_INVALID_ROLE, the Low Power feature is not supported.
    @note When finish establishing friendship, an event #BT_MESH_EVT_FRIENDSHIP_STATUS is received.
*/
bt_mesh_status_t bt_mesh_lpn_establish_friendship(
    uint16_t friend_addr,
    uint16_t netkey_index);

/*!
    @brief Terminate the existing friendship.
    @return
    #BT_MESH_SUCCESS, the request is sent successfully.\n
    #BT_MESH_ERROR_INVALID_STATE, no friendship exist.\n
    #BT_MESH_ERROR_INVALID_ROLE, the Low Power feature is not supported.
    @note When the friendship is terminated, an event #BT_MESH_EVT_FRIENDSHIP_STATUS is received.
*/
bt_mesh_status_t bt_mesh_lpn_terminate_friendship(void);

/*!
    @brief Add a subscription list request to Friend node.
    @param[in] transaction_number is the transaction number for this operation.
    @param[in] addr_list is the list of addresses to be added.
    @param[in] list_count is the length of addr_list.
    @return
    #BT_MESH_SUCCESS, the request is sent successfully.\n
    #BT_MESH_ERROR_INVALID_STATE, no friendship exist.\n
    #BT_MESH_ERROR_INVALID_PARAM, the address list is empty or list count is zero.\n
    #BT_MESH_ERROR_INVALID_ROLE, the Low Power feature is not supported.
    @note When the command is received by Friend node, an event #BT_MESH_EVT_LPN_FRIEND_SUBSCRIPTION_LIST_CONFRIM is received.
*/
bt_mesh_status_t bt_mesh_lpn_add_subscription_list(
    uint8_t transaction_number,
    const uint16_t *addr_list,
    uint8_t list_count);

/*!
    @brief Remove a subscription list request to Friend node.
    @param[in] transaction_number is the transaction number for this operation.
    @param[in] addr_list is the list of addresses to be removed.
    @param[in] list_count is the length of addr_list.
    @return
    #BT_MESH_SUCCESS, the request is sent successfully.\n
    #BT_MESH_ERROR_INVALID_STATE, no friendship exist.\n
    #BT_MESH_ERROR_INVALID_PARAM, the address list is empty or list count is zero.\n
    #BT_MESH_ERROR_INVALID_ROLE, the Low Power feature is not supported.
    @note When the command is received by Friend node, an event #BT_MESH_EVT_LPN_FRIEND_SUBSCRIPTION_LIST_CONFRIM is received.
*/
bt_mesh_status_t bt_mesh_lpn_remove_subscription_list(
    uint8_t transaction_number,
    const uint16_t *addr_list,
    uint8_t list_count);

/*!
    @brief Set Low Power node resend poll message times.
    @param[in] resend_times is the poll message resend times when Low Power node dose not receive a response within the ReceiveWindow from Friend node. The default resend_times is 3.
    @return
    #BT_MESH_SUCCESS, the command is set successfully.\n
    #BT_MESH_ERROR_INVALID_ROLE, the Low Power feature is not supported.
*/
bt_mesh_status_t bt_mesh_lpn_set_poll_resend_times(uint8_t resend_times);

/*!
    @brief Set Low Power node to Low Power mode and disable Mesh function.
    @return
    #BT_MESH_SUCCESS, the command start successfully.\n
    #BT_MESH_ERROR_FAIL, the command failed.\n
    #BT_MESH_ERROR_INVALID_STATE, the operation is in process, or no friendship exist.\n
    #BT_MESH_ERROR_INVALID_ROLE, the Low Power feature is not supported.
    @note When the command is completed, an event #BT_MESH_EVT_LOW_POWER_STATUS is received.
*/
bt_mesh_status_t bt_mesh_lpn_enable_low_power_mode(void);

/*!
    @brief Reset Low Power node to normal mode and enable Mesh function.
    @return
    #BT_MESH_SUCCESS, the command start successfully.\n
    #BT_MESH_ERROR_FAIL, the command failed.\n
    #BT_MESH_ERROR_INVALID_STATE, the request is in process, or no friendship exist.\n
    #BT_MESH_ERROR_INVALID_ROLE, the Low Power feature is not supported.
    @note When the command is completed, an event #BT_MESH_EVT_LOW_POWER_STATUS is received.
*/
bt_mesh_status_t bt_mesh_lpn_disable_low_power_mode(void);


/*!
@}
@}
*/

#endif // __BT_MESH_LPN_H__
