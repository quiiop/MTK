/* Copyright Statement:
 *
 * (C) 2020  MediaTek Inc. All rights reserved.
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



#ifndef __BLE_BAP_CLIENT_H__
#define __BLE_BAP_CLIENT_H__

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup Bluetooth BLE BAP CLIENT
 * @{
 * This section introduces the BAP operation codes, request and response structures, API prototypes.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b BAP                        | Basic Audio Profile. |
 * |\b ASE                        | Audio Stream Endpoint. |
 * |\b CIS                        | Connected Isochronous Stream. |
 */

#ifndef PACKED
#define PACKED  __attribute__((packed))
#endif

#include "bt_le_audio_def.h"
#include "ble_ascs_def.h"
#include "bt_system.h"

/**
 * @defgroup Bluetoothbt_BAP_define Struct
 * @{
 * This section defines basic data structures for the BAP.
 */
#define BLE_BAP_PACS_READ_SINK_PAC_CNF                                0x01
#define BLE_BAP_PACS_READ_SINK_LOCATION_CNF                           0x02
#define BLE_BAP_PACS_READ_SOURCE_PAC_CNF                              0x03
#define BLE_BAP_PACS_READ_SOURCE_LOCATION_CNF                         0x04
#define BLE_BAP_PACS_READ_AVAILABLE_AUDIO_CONTEXTS_CNF                0x05
#define BLE_BAP_PACS_READ_SUPPORTED_AUDIO_CONTEXTS_CNF                0x06
#define BLE_BAP_PACS_SET_SINK_PAC_NOTIFICATION_CNF                    0x07
#define BLE_BAP_PACS_SET_SINK_LOCATION_NOTIFICATION_CNF               0x08
#define BLE_BAP_PACS_SET_SOURCE_PAC_NOTIFICATION_CNF                  0x09
#define BLE_BAP_PACS_SET_SOURCE_LOCATION_NOTIFICATION_CNF             0x0A
#define BLE_BAP_PACS_SET_AVAILABLE_AUDIO_CONTEXTS_NOTIFICATION_CNF    0x0B
#define BLE_BAP_PACS_SET_SUPPORTED_AUDIO_CONTEXTS_NOTIFICATION_CNF    0x0C
#define BLE_BAP_PACS_SINK_PAC_NOTIFY                                  0x0D
#define BLE_BAP_PACS_SINK_LOCATION_NOTIFY                             0x0E
#define BLE_BAP_PACS_SOURCE_PAC_NOTIFY                                0x0F
#define BLE_BAP_PACS_SOURCE_LOCATION_NOTIFY                           0x10
#define BLE_BAP_PACS_AVAILABLE_AUDIO_CONTEXTS_NOTIFY                  0x11
#define BLE_BAP_PACS_SUPPORTED_AUDIO_CONTEXTS_NOTIFY                  0x12
#define BLE_BAP_PACS_DISCOVER_SERVICE_COMPLETE_NOTIFY                 0x13
#define BLE_BAP_ASCS_READ_ASE_CNF                                     0x14
#define BLE_BAP_ASCS_SET_ASE_NOTIFICATION_CNF                         0x15
#define BLE_BAP_ASCS_SET_ASCS_ASE_CONTROL_POINT_NOTIFICATION_CNF      0x16
#define BLE_BAP_ASCS_ASE_NOTIFY                                       0x17
#define BLE_BAP_ASCS_ASE_CONTROL_POINT_NOTIFY                         0x18
#define BLE_BAP_ASCS_DISCOVER_SERVICE_COMPLETE_NOTIFY                 0x19
typedef uint8_t ble_bap_event_t;                                      /**< The type of BAP events.*/

/**
 *  @brief This structure defines the codec id parameters used in #ble_bap_config_codec_param_t.
 */
typedef struct {
    uint8_t coding_format;                      /**< The Coding_Format. */
    uint16_t company_id;                        /**< The company ID. */
    uint16_t vendor_specific_codec_id ;         /**< The Vendor-specific codec ID  */
} PACKED ble_bap_codec_id_t;

/**
 *  @brief This structure defines the parameter data type used in #ble_bap_config_codec_param_t.
 */
typedef struct {
    uint8_t sampling_freq_len;                  /**< The length of sampling frequency LTV structure. */
    uint8_t sampling_freq_type;                 /**< The type of sampling frequency LTV structure. */
    uint8_t sampling_freq_value;                /**< The length of sampling frequency LTV structure. */

    uint8_t frame_duration_len;                 /**< The length of frame duration LTV structure. */
    uint8_t frame_duration_type;                /**< The type of frame duration LTV structure. */
    uint8_t frame_duration_value;               /**< The value of frame duration LTV structure. */

    uint8_t octets_per_codec_frame_len;         /**< The length of octets per codec frame LTV structure. */
    uint8_t octets_per_codec_frame_type;        /**< The type of octets per codec frame LTV structure. */
    uint16_t octets_per_codec_frame_value;      /**< The value of octets per codec frame LTV structure. */

    uint8_t audio_channel_alloaction_len;       /**< The length of audio channel allocation LTV structure. */
    uint8_t audio_channel_alloaction_type;      /**< The type of audio channel allocation LTV structure. */
    uint32_t audio_channel_alloaction_value;    /**< The value of audio channel allocation LTV structure. */
} PACKED ble_bap_codec_specific_configuration_param_t;

/**
 *  @brief This structure defines the parameter data type used in #ble_bap_enable_param_t and #ble_bap_update_metadata_param_t.
 */
typedef struct {
    uint8_t metadata_len;                       /**< The length of metadata LTV structure. */
    uint8_t metadata_type;                      /**< The type of metadata LTV structure. */
    uint16_t metadata_value;                    /**< The value of metadata LTV structure.  */
} PACKED ble_bap_metadata_param_t;

/**
 *  @brief This structure defines the parameter data type used in structure #ble_bap_ascs_config_codec_operation_t.
 */
typedef struct {
    uint8_t ase_id;                                                                 /**< The ASE_ID for this ASE. */
    //uint8_t direction;                                                              /**< The direction of this ASE with respect to the server. */
    uint8_t target_latency;                                                         /**< The target latency. */
    uint8_t target_phy;                                                             /**< PHY parameter target to achieve the Target_Latency value. */
    ble_bap_codec_id_t codec_id;                                                    /**< The codec ID. */
    uint8_t codec_specific_configuration_length;                                    /**< Length of the Codec_Specific_Configuration value for this ASE. */
    ble_bap_codec_specific_configuration_param_t codec_specific_configuration;      /**< Codec-Specific Configuration for this ASE. */
} PACKED ble_bap_config_codec_param_t;

/**
 *  @brief This structure defines the parameter data type used in structure #ble_bap_ascs_config_qos_operation_t.
 */
typedef struct {
    uint8_t  ase_id;                                        /**< The ASE_ID for this ASE. */
    uint8_t  cig_id;                                        /**< CIG ID parameter value written by the client for this ASE. */
    uint8_t  cis_id;                                        /**< CIS ID parameter value written by the client for this ASE. */
    uint32_t sdu_interval: 24;                              /**< SDU_Interval parameter value written by the client for this ASE. */
    uint8_t  framing;                                       /**< Framing parameter value written by the client for this ASE. */
    uint8_t  phy;                                           /**< PHY parameter value written by the client for this ASE. */
    uint16_t maximum_sdu_size;                              /**< Maximum SDU parameter value written by the client for this ASE. */
    uint8_t  retransmission_number;                         /**< Retransmission number parameter value written by the client for this ASE. */
    uint16_t transport_latency;                             /**< Transport latency parameter value written by the client for this ASE. */
    uint32_t presentation_delay: 24;                        /**< Presentation delay parameter value written by the client for this ASE. */
} PACKED ble_bap_config_qos_param_t;                        /**< Presentation delay parameter value written by the client for this ASE. */

/**
 *  @brief This structure defines the parameter data type used in structure #ble_bap_ascs_enable_operation_t and #ble_bap_ascs_update_metadata_operation_t.
 */
typedef struct {
    uint8_t  ase_id;                                        /**< The ASE_ID for this ASE. */
    uint8_t  metadata_length;                               /**< Length of the Metadata parameter for this ASE. */
    ble_bap_metadata_param_t  metadata;                             /**< LTV-formatted Metadata for this ASE. */
} PACKED ble_bap_enable_param_t, ble_bap_update_metadata_param_t;

/**
 *  @brief This structure defines the parameter data type used in structure #ble_bap_ascs_disable_operation_t, #ble_bap_ascs_release_operation_t, #ble_bap_ascs_receiver_start_ready_operation_t and #ble_bap_ascs_receiver_stop_ready_operation_t.
 */
typedef struct {
    uint8_t  ase_id;                                /**< The ASE_ID for this ASE. */
} ble_bap_disable_param_t, ble_bap_release_param_t, ble_bap_receiver_start_ready_param_t, ble_bap_receiver_stop_ready_param_t;

/**
 *  @brief This structure defines the parameter data type used in function #ble_bap_ascs_config_codec().
 */
typedef struct {
    uint8_t opcode;                                 /**< Opcode of the client-initiated ASE Control operation causing this response. */
    uint8_t num_of_ase;                             /**< Total number of ASEs used in the Config Codec operation. */
    ble_bap_config_codec_param_t param[1];          /**< The parameters of Config Codec operation. */
} PACKED ble_bap_ascs_config_codec_operation_t;

/**
 *  @brief This structure defines the parameter data type used in function #ble_bap_ascs_config_qos().
 */
typedef struct {
    uint8_t opcode;                                 /**< Opcode of the client-initiated ASE Control operation causing this response. */
    uint8_t num_of_ase;                             /**< Total number of ASEs used in the Config QoS operation. */
    ble_bap_config_qos_param_t param[1];            /**< The parameters of Config QoS operation. */
} PACKED ble_bap_ascs_config_qos_operation_t;

/**
 *  @brief This structure defines the parameter data type used in function #ble_bap_ascs_enable_ase_ex().
 */
typedef struct {
    uint8_t opcode;                                 /**< Opcode of the client-initiated ASE Control operation causing this response. */
    uint8_t num_of_ase;                             /**< Total number of ASEs used in the Enable operation. */
    ble_bap_enable_param_t param[1];                /**< The parameters of Enable operation. */
} PACKED ble_bap_ascs_enable_operation_t;

/**
 *  @brief This structure defines the parameter data type used in function #ble_bap_ascs_receiver_start_ready().
 */
typedef struct {
    uint8_t opcode;                                 /**< Opcode of the client-initiated ASE Control operation causing this response. */
    uint8_t num_of_ase;                             /**< Total number of ASE_IDs used in the Receiver Start Ready operation. */
    ble_bap_receiver_start_ready_param_t param[1];  /**< The parameters of Receiver Start Ready operation. */
} PACKED ble_bap_ascs_receiver_start_ready_operation_t;

/**
 *  @brief This structure defines the parameter data type used in function #ble_bap_ascs_disable_ase().
 */
typedef struct {
    uint8_t opcode;                                 /**< Opcode of the client-initiated ASE Control operation causing this response. */
    uint8_t num_of_ase;                             /**< Total number of ASE_IDs used in the Disable operation. */
    ble_bap_disable_param_t param[1];               /**< The parameters of Disable operation. */
} PACKED ble_bap_ascs_disable_operation_t;

/**
 *  @brief This structure defines the parameter data type used in function #ble_bap_ascs_receiver_stop_ready().
 */
typedef struct {
    uint8_t opcode;                                 /**< Opcode of the client-initiated ASE Control operation causing this response. */
    uint8_t num_of_ase;                             /**< Total number of ASE_IDs used in the Receiver Stop Ready operation. */
    ble_bap_receiver_stop_ready_param_t param[1];   /**< The parameters of Receiver Stop Ready operation. */
} PACKED ble_bap_ascs_receiver_stop_ready_operation_t;

/**
 *  @brief This structure defines the parameter data type used in function #ble_bap_ascs_update_metadata_ex().
 */
typedef struct {
    uint8_t opcode;                                 /**< Opcode of the client-initiated ASE Control operation causing this response. */
    uint8_t num_of_ase;                             /**< Total number of ASE_IDs used in the Update Metadata operation */
    ble_bap_update_metadata_param_t param[1];       /**< The parameters of Update Metadata operation. */
} PACKED ble_bap_ascs_update_metadata_operation_t;

/**
 *  @brief This structure defines the parameter data type used in function #ble_bap_ascs_release_ase().
 */
typedef struct {
    uint8_t opcode;                                 /**< Opcode of the client-initiated ASE Control operation causing this response. */
    uint8_t num_of_ase;                             /**< Total number of ASE_IDs used in the Release operation */
    ble_bap_release_param_t param[1];               /**< The parameters of Release operation. */
} PACKED ble_bap_ascs_release_operation_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_BAP_PACS_READ_SINK_PAC_CNF.
 */
typedef struct {
    bt_handle_t handle;                                 /**< Connection handle. */
    uint8_t num_of_record;                              /**< Number of PAC records in this characteristic. */
    uint8_t pac_record_length;                          /**< Length of PAC records. */
    uint8_t *pac_record;                                /**< Data of PAC records. */
} ble_bap_read_sink_pac_cnf_t;

typedef ble_bap_read_sink_pac_cnf_t ble_bap_sink_pac_notify_t;
typedef ble_bap_read_sink_pac_cnf_t ble_bap_read_source_pac_cnf_t;
typedef ble_bap_read_sink_pac_cnf_t ble_bap_source_pac_notify_t;

/**
 *  @brief This structure defines the parameter data type for event #BLE_BAP_PACS_READ_SINK_LOCATION_CNF.
 */
typedef struct {
    bt_handle_t handle;                                 /**< Connection handle. */
    uint32_t location;                                  /**< Sink location. */
} ble_bap_read_sink_location_cnf_t;

typedef ble_bap_read_sink_location_cnf_t ble_bap_sink_location_notify_t;
typedef ble_bap_read_sink_location_cnf_t ble_bap_read_source_location_cnf_t;
typedef ble_bap_read_sink_location_cnf_t ble_bap_source_location_notify_t;

typedef struct {
    bt_handle_t handle;                                 /**< Connection handle. */
    uint16_t sink_contexts;                             /**< Length of the Metadata parameter for this PAC record. */
    uint16_t source_contexts;                           /**< Length of the Metadata parameter for this PAC record. */
} ble_bap_read_available_audio_contexts_cnf_t;

typedef ble_bap_read_available_audio_contexts_cnf_t ble_bap_available_audio_contexts_notify_t;
typedef ble_bap_read_available_audio_contexts_cnf_t ble_bap_read_supported_audio_contexts_cnf_t;
typedef ble_bap_read_available_audio_contexts_cnf_t ble_bap_supported_audio_contexts_notify_t;

/**
 *  @brief This structure defines the read ASE state cnf.
 */
typedef struct {
    bt_handle_t handle;                             /**< Connection handle. */
    uint8_t  ase_id;                                /**< Identifier of this ASE. */
    uint8_t  ase_state;                             /**< State of the ASE with respect to the ASE state machine. */
    uint8_t  *additional_parameter;                 /**< The Additional ASE Parameters. */
} PACKED ble_ascs_read_ase_cnf_t;

typedef struct {
    bt_handle_t handle;                             /**< Connection handle. */
    uint8_t  direction;                             /**< Direction. */
    uint8_t  ase_id;                                /**< Identifier of this ASE. */
    uint8_t  ase_state;                             /**< State of the ASE with respect to the ASE state machine. */
    uint8_t  *additional_parameter;                 /**< The Additional ASE Parameters. */
} PACKED ble_bap_ase_notify_t;

typedef struct {
    uint8_t  ase_id;                                /**< Identifier of this ASE. */
    uint8_t  response_code;                         /**< State of the ASE with respect to the ASE state machine. */
    uint8_t  reason;                                /**< The Additional ASE Parameters. */
} PACKED ble_ascs_response_t;

typedef struct {
    bt_handle_t handle;                              /**< Connection handle. */
    uint8_t  opcode;                                 /**< Identifier of this ASE. */
    uint8_t  number_of_ases;                         /**< Number of ASEs. */
    ble_ascs_response_t *response;                   /**< The Additional ASE Parameters. */
} PACKED ble_ascs_control_point_notify_t;

typedef struct {
    bt_handle_t handle;               /**< Connection handle. */
    bt_status_t status;               /**< Event status. */
    uint8_t  number_of_sink_ases;     /**< Number of sink ASEs. */
    uint8_t  number_of_source_ases;   /**< Number of source ASEs. */
} ble_bap_discover_service_complete_t;

/**
 * @brief This structure defines the event callback for BAP.
 * @param[in] event             is the event id.
 * @param[in] msg               is the event message.
 */
typedef void (*ble_bap_callback_t)(ble_bap_event_t event, void *msg);


/**
 * @}
 */


BT_EXTERN_C_BEGIN

/**
 * @brief               This function initiates codec config operation to the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @param[in] param     is the parameters of codec config operation.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_ascs_config_codec(bt_handle_t handle, ble_bap_ascs_config_codec_operation_t *param);

/**
 * @brief               This function initiates codec qos operation to the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @param[in] param     is the parameters of qos config operation.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_ascs_config_qos(bt_handle_t handle, ble_bap_ascs_config_qos_operation_t *param);

/**
 * @brief               This function initiates the enabling operation for the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @param[in] param     is the parameters of the enabling operation.
 * @param[in] length    is the length of the enabling command.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_ascs_enable_ase_ex(bt_handle_t handle, uint8_t *param, uint16_t length);

/**
 * @deprecated Please use ble_bap_ascs_enable_ase_ex instead.
 * @brief               This function initiates the enabling operation for the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @param[in] param     is the parameters of the enabling operation.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_ascs_enable_ase(bt_handle_t handle, ble_bap_ascs_enable_operation_t *param);


/**
 * @brief               This function initiates receiver start ready operation to the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @param[in] param     is the parameters of receiver start ready operation.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_ascs_receiver_start_ready(bt_handle_t handle, ble_bap_ascs_receiver_start_ready_operation_t *param);

/**
 * @brief               This function initiates disable operation to the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @param[in] param     is the parameters of disable operation.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_ascs_disable_ase(bt_handle_t handle, ble_bap_ascs_disable_operation_t *param);
/**
 * @brief               This function initiates receiver stop ready operation to the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @param[in] param     is the parameters of receiver stop ready operation.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_ascs_receiver_stop_ready(bt_handle_t handle, ble_bap_ascs_receiver_stop_ready_operation_t *param);

/**
 * @brief               This function initiates the update metadata operation for the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @param[in] param     is the parameters of the update metadata operation.
 * @param[in] length    is the length of the update metadata command.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_ascs_update_metadata_ex(bt_handle_t handle, uint8_t *param, uint16_t length);

/**
 * @deprecated Please use ble_bap_ascs_update_metadata_ex instead.
 * @brief               This function initiates the update metadata operation for the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @param[in] param     is the parameters of the update metadata operation.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_ascs_update_metadata(bt_handle_t handle, ble_bap_ascs_update_metadata_operation_t *param);


/**
 * @brief               This function initiates release operation to the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @param[in] param     is the parameters of release operation.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_ascs_release_ase(bt_handle_t handle, ble_bap_ascs_release_operation_t *param);

/**
 * @brief               This function reads ASE values of the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @param[in] ase_id    is the ASE ID to read.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_ascs_read_ase_value_req(bt_handle_t handle, uint8_t ase_id);

/**
 * @brief               This function reads sink PAC of the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_pacs_read_sink_pac_req(bt_handle_t handle);

/**
 * @brief               This function reads the sink location of the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_pacs_read_sink_location_req(bt_handle_t handle);

/**
 * @brief               This function reads the source PAC of the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_pacs_read_source_pac_req(bt_handle_t handle);

/**
 * @brief               This function reads the source location of the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_pacs_read_source_location_req(bt_handle_t handle);

/**
 * @brief               This function reads the available audio contexts of the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_pacs_read_available_audio_contexts_req(bt_handle_t handle);

/**
 * @brief               This function reads the supported audio contexts of the specified remote device.
 * @param[in] handle    is the connection handle of the Bluetooth link.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_pacs_read_supported_audio_contexts_req(bt_handle_t handle);

/**
 * @brief                   This function initializes the BAP client.
 * @param[in] callback      is the callback function of receiving the BAP client events.
 * @param[in] max_link_num  is the maximum number of link.
 * @return              #BT_STATUS_SUCCESS, the operation completed successfully.
 *                      #BT_STATUS_FAIL, the operation has failed.
 */

bt_status_t ble_bap_client_init(ble_bap_callback_t callback, uint8_t max_link_num);

bt_status_t ble_bap_reset_service_attribute(bt_handle_t handle);

BT_EXTERN_C_END
/**
 * @}
 * @}
 */

#endif


