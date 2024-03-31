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

#ifndef __BLE_ASCS_DEF_H__
#define __BLE_ASCS_DEF_H__

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup Bluetooth ASCS
 * @{
 * This section introduces the ASCS operation codes, request and response structures, API prototypes.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b ASE                        | Audio Stream Endpoint. |
 */

#ifndef PACKED
#define PACKED  __attribute__((packed))
#endif

#include "bt_le_audio_def.h"

/**
 * @brief The ASCS service UUID.
 */
#define BT_GATT_UUID16_ASCS_SERVICE             (0x184E)    /**< Audio Stream Control Service UUID. */

/**
* @brief ASCS characteristic UUID.
*/
#define BT_SIG_UUID16_SINK_ASE                  (0x2BC4)    /**< Sink Audio Stream Endpoint Characteristic UUID. */
#define BT_SIG_UUID16_SOURCE_ASE                (0x2BC5)    /**< Source Audio Stream Endpoint Characteristic UUID. */
#define BT_SIG_UUID16_ASE_CONTROL_POINT         (0x2BC6)    /**< Audio Stream Endpoint Control Point Characteristic UUID. */

/**
* @brief ASCS Configurations.
*/
#define BLE_ASCS_FRAMING_UNFRAMED    0x00                    /**< Unframed  */
#define BLE_ASCS_FRAMING_FRAMED      0x01                    /**< Framed  */

#define BLE_ASCS_PHY_1M          0x01                        /**< LE 1M PHY*/
#define BLE_ASCS_PHY_2M          0x02                        /**< LE 2M PHY*/
#define BLE_ASCS_PHY_CODED       0x04                        /**< LE CODED PHY*/

#define ASCS_OCTETS_PER_CODEC_FRAME_30      0x001E      /**< 30 Octets per codec frame */
#define ASCS_OCTETS_PER_CODEC_FRAME_40      0x0028      /**< 40 Octets per codec frame */
#define ASCS_OCTETS_PER_CODEC_FRAME_45      0x002D      /**< 45 Octets per codec frame */
#define ASCS_OCTETS_PER_CODEC_FRAME_60      0x003C      /**< 60 Octets per codec frame */
#define ASCS_OCTETS_PER_CODEC_FRAME_75      0x004B      /**< 75 Octets per codec frame */
#define ASCS_OCTETS_PER_CODEC_FRAME_80      0x0050      /**< 80 Octets per codec frame */
#define ASCS_OCTETS_PER_CODEC_FRAME_90      0x005A      /**< 90 Octets per codec frame */
#define ASCS_OCTETS_PER_CODEC_FRAME_100     0x0064      /**< 100 Octets per codec frame */
#define ASCS_OCTETS_PER_CODEC_FRAME_117     0x0075      /**< 117 Octets per codec frame */
#define ASCS_OCTETS_PER_CODEC_FRAME_120     0x0078      /**< 120 Octets per codec frame */
#define ASCS_OCTETS_PER_CODEC_FRAME_155     0x009B      /**< 155 Octets per codec frame */

/**
 * @brief The ASCS UUID type definitions.
 */
#define BLE_ASCS_UUID_TYPE_ASCS_SERVICE          0                   /**< ASCS UUID type.*/
#define BLE_ASCS_UUID_TYPE_SINK_ASE              1                   /**< Sink ASE UUID type.*/
#define BLE_ASCS_UUID_TYPE_SOURCE_ASE            2                   /**< Source ASE UUID type.*/
#define BLE_ASCS_UUID_TYPE_ASE_CONTROL_POINT     3                   /**< ASE Control Point UUID type.*/
#define BLE_ASCS_UUID_TYPE_MAX_NUM               4                   /**< The max number of ASCS UUID type.*/
#define BLE_ASCS_UUID_TYPE_INVALID               0xFF                /**< The invalid ASCS UUID type.*/
typedef uint8_t ble_ascs_uuid_t;                                     /**< UUID type.*/

#define BLE_ASCS_UUID_TYPE_CHARC_START           BLE_ASCS_UUID_TYPE_SINK_ASE

/**
 * @brief PRESENTATION DELAY.
 */
#define SUPPORTED_PRESENTATION_DELAY_MAX               (40000)
#define SUPPORTED_PRESENTATION_DELAY_MIN               (20000)
#define PREFERRED_PRESENTATION_DELAY_MAX               (40000)
#define PREFERRED_PRESENTATION_DELAY_MIN               (20000)

/**
 * @brief The ASE state.
 */
enum {
    ASE_STATE_IDLE = 0x00,                                      /**< Idle */
    ASE_STATE_CODEC_CONFIGURED,                                 /**< Codec Configured */
    ASE_STATE_QOS_CONFIGURED,                                   /**< Qos Configured */
    ASE_STATE_ENABLING,                                         /**< Enabling */
    ASE_STATE_STREAMING,                                        /**< Streaming */
    ASE_STATE_DISABLING, //0x05                                 /**< Disabling */
    ASE_STATE_RELEASING,                                        /**< Releasing */
};

/**
 * @brief The ASE Control Point opcode.
 */
enum {
    ASE_OPCODE_CONFIG_CODEC             = 0x01,                 /**< Config Codec operation */
    ASE_OPCODE_CONFIG_QOS               = 0x02,                 /**< Config QoS operation  */
    ASE_OPCODE_ENABLE                   = 0x03,                 /**< Enable operation */
    ASE_OPCODE_RECEIVER_START_READY     = 0x04,                 /**< Receiver Start Ready operation */
    ASE_OPCODE_DISABLE                  = 0x05,                 /**< Disable operation */
    ASE_OPCODE_RECEIVER_STOP_READY      = 0x06,                 /**< Receiver Stop Ready operation */
    ASE_OPCODE_UPDATE_METADATA          = 0x07,                 /**< Update Metadata operation */
    ASE_OPCODE_RELEASE                  = 0x08,                 /**< Release operation */
    ASE_OPCODE_RESERVED_FUTURE,
};

/**
 * @brief ASE Control Point characteristic Response Code.
 */
enum {
    RESPONSE_CODE_SUCCESS,                                      /**< The server has successfully completed the client-initiated ASE Control operation.  */
    RESPONSE_CODE_UNSUPPORTED_OPCODE,                           /**< The server does not support the client-initiated ASE Control operation defined by the opcode written by the client.  */
    RESPONSE_CODE_INVALID_LENGTH,                               /**< The server has detected a invalid length operation written by the client.  */
    RESPONSE_CODE_INVALID_ASE_ID,                               /**< The server has detected that an ASE_ID written by the client does not match an ASE_ID in an exposed ASE characteristic value for that client.  */
    RESPONSE_CODE_INVALID_ASE_STATE_MACHINE_TRANSITION,         /**< The server has detected that the client-initiated ASE Control operation would cause an invalid ASE state machine transition.  */
    RESPONSE_CODE_INVALID_ASE_DIRECTION,                        /**< The server has detected that the client-initiated ASE Control operation is not valid for the ASE direction.   */
    RESPONSE_CODE_UNSUPPORTED_AUDIO_CAPABILITIES,               /**< The server has detected that the audio capabilities requested during a Config Codec operation are not supported.  */
    RESPONSE_CODE_UNSUPPORTED_PARAMETER_VALUE,                  /**< The server has detected it does not support one or more configuration parameter values written by the client.  */
    RESPONSE_CODE_REJECTED_PARAMETER_VALUE,                     /**< The server has rejected one or more configuration parameter values written by the client. */
    RESPONSE_CODE_INVALID_PARAMETER_VALUE,                      /**< The server has detected one or more invalid configuration parameter values written by the client.  */
    RESPONSE_CODE_UNSUPPORTED_METADATA,                         /**< The server has detected an unsupported Metadata Type written by the client.  */
    RESPONSE_CODE_REJECTED_METADATA,                            /**< The server has rejected a Metadata Type written by the client. */
    RESPONSE_CODE_INVALID_METADATA,                             /**< This Response_Code is used to inform the client that the Metadata Value is incorrectly formatted.  */
    RESPONSE_CODE_INSUFFICIENT_RESOURCES,                       /**< The server is unable to successfully complete the client-initiated ASE Control operation because of insufficient resources.  */
    RESPONSE_CODE_UNSPECIFIED_ERROR,                            /**< The server has encountered an unspecified error. */
    RESPONSE_CODE_MAX,
};

enum {
    ERROR_REASON_NO_ERROR,                                      /**< No error */
    //ERROR_REASON_DIRECTION,                                     /**< Direction */  //Delete in ASCS_Validation_r09
    ERROR_REASON_CODEC_ID,                                      /**< Codec_ID */
    //ERROR_REASON_CODEC_SPECIFIC_CONFIGURATION_LEN,              /**< Codec_Specific_Configuration_Length */  //Delete in ASCS_Validation_r09
    ERROR_REASON_CODEC_SPECIFIC_CONFIGURATION,                  /**< Codec_Specific_Configuration */
    ERROR_REASON_SDU_INTERVAL,                                  /**< SDU_Interval */
    ERROR_REASON_FRAMING,                                       /**< Framing */
    ERROR_REASON_PHY,                                           /**< PHY */
    ERROR_REASON_MAXIMUM_SDU_SIZE,                              /**< Maximum_SDU_Size */
    ERROR_REASON_RETRANSMISSION_NUMBER,                         /**< Retransmission_Number */
    ERROR_REASON_MAX_TRANSPORT_LATENCY,                         /**< Max_Transport_Latency */
    ERROR_REASON_PRESENTATION_DELAY,                            /**< Presentation_Delay */
    ERROR_REASON_INVALID_ASE_CIS_MAPPING,                       /**< Invalid_ASE_CIS_Mapping */
    //ERROR_REASON_METADATA_LEN,                                  /**< Metadata_Length */  //Delete in ASCS_Validation_r09
    ERROR_REASON_MAX,
};

/**
 * @brief ASE target latency.
 */
enum {
    TARGET_LOWER_LATENCY = 0x01,
    TARGET_BALANCED_LATENCY_AND_RELIABILITY,
    TARGET_HIGHER_RELIABILITY
};

/**
 * @brief ASE Qos preference index.
 */
enum {
    QOS_32KBPS_SETTING,     /**< 32kbps, Mandatory */
    QOS_48KBPS_SETTING,     /**< 48kbps, Mandatory */
    QOS_64KBPS_SETTING,     /**< 64kbps */
    QOS_80KBPS_SETTING,     /**< 80kbps, */
    QOS_96KBPS_SETTING,     /**< 96kbps, */
    QOS_124KBPS_SETTING,    /**< 124kbps, */
};

/**
 * @brief The ASCS max number of characteristics.
 */
#define BLE_ASCS_MAX_CHARC_NUMBER    (BLE_ASCS_UUID_TYPE_MAX_NUM-1)   /**< The number of ASCS characteristics.*/

/**
 * @brief The ASCS GATT type definitions.
 */
#define BLE_ASCS_READ_ASE                       0x00    /**< Read ASE*/
#define BLE_ASCS_READ_ASE_CCCD                  0x01    /**< Read ASE CCCD */
#define BLE_ASCS_WRITE_ASE_CCCD                 0x02    /**< Write ASE CCCD */
#define BLE_ASCS_WRITE_ASE_CONTROL_POINT        0x03    /**< Write ASE Control Point */
#define BLE_ASCS_READ_ASE_CONTROL_POINT_CCCD    0x04    /**< Read ASE Control Point CCCD */
#define BLE_ASCS_WRITE_ASE_CONTROL_POINT_CCCD   0x05    /**< Write ASE Control Point CCCD */
#define BLE_ASCS_GATTS_REQ_MAX                  0x06    /**< The maximum number of ASCS GATT type. */
typedef uint8_t ble_ascs_gatt_request_t;


/**
 * @defgroup Bluetoothbt_BAP_define Struct
 * @{
 * This section defines basic data structures for the BAP.
 */

/**
 *  @brief This structure defines the ASE characteristic detail.
 */
typedef struct {
    uint8_t  ase_id;                                /**< Identifier of this ASE. */
    uint8_t  ase_state;                             /**< State of the ASE with respect to the ASE state machine. */
    uint8_t  parameter[1];                          /**< The Additional ASE Parameters. */
} PACKED ble_ascs_audio_stream_endpoint_t;

/**
 *  @brief This structure defines the parameters in config codec operation.
 */
typedef struct {
    uint8_t ase_id;                                              /**< Identifier of this ASE. */
    uint8_t target_latency;                                      /**< The target latency. */
    uint8_t target_phy;                                          /**< PHY parameter target to achieve the Target_Latency value. */
    uint8_t codec_id[AUDIO_CODEC_ID_SIZE];                       /**< The codec id. */
    uint8_t codec_specific_configuration_length;                 /**< Length of the Codec_Specific_Configuration value for this ASE. */
    uint8_t codec_specific_configuration[1];                     /**< Codec-Specific Configuration for this ASE. */
} PACKED ble_ascs_config_codec_operation_t;


/**
 *  @brief This structure defines the parameters in config codec operation indication.
 */
typedef struct {
    bt_le_audio_direction_t direction;                           /**< Audio direction this ASE. */
    ble_ascs_config_codec_operation_t *param;                    /**< Parameters of codec configuration for this ASE. */
} PACKED ble_ascs_config_codec_operation_ind_t;

/**
 *  @brief This structure defines the parameters in update metadata operation.
 */

typedef struct {
    uint8_t ase_id;                                             /**< Identifier of this ASE. */
    uint8_t metadata_length;                                    /**< Length of the metadata for this ASE. */
    uint8_t metadata[1];                                        /**< Metadata for this ASE. */
} PACKED ble_ascs_update_metadata_operation_t;

/**
 *  @brief This structure defines the parameters in config QoS operation.
 */
typedef struct {
    uint8_t  ase_id;                                            /**< Identifier of this ASE. */
    uint8_t  cig_id;                                            /**< CIG ID parameter value written by the client for this ASE. */
    uint8_t  cis_id;                                            /**< CIS ID parameter value written by the client for this ASE. */
    uint8_t  sdu_interval[SDU_INTERVAL_SIZE];                   /**< SDU_Interval parameter value written by the client for this ASE. */
    uint8_t  framing;                                           /**< Framing parameter value written by the client for this ASE. */
    uint8_t  phy;                                               /**< PHY parameter value written by the client for this ASE. */
    uint16_t maximum_sdu_size;                                  /**< Maximum SDU parameter value written by the client for this ASE. */
    uint8_t  retransmission_number;                             /**< Retransmission number parameter value written by the client for this ASE. */
    uint16_t transport_latency;                                 /**< Transport latency parameter value written by the client for this ASE. */
    uint8_t presentation_delay[PRESENTATION_DELAY_SIZE];        /**< Presentation delay parameter value written by the client for this ASE. */
} PACKED ble_ascs_config_qos_operation_t;

/**
 *  @brief This structure defines ASCS attribute handle detail.
 */
typedef struct {
    ble_ascs_uuid_t uuid_type;  /**< UUID type */
    uint16_t att_handle;        /**< Attribute handle */
} ble_ascs_attribute_handle_t;

/**
 *  @brief This structure defines ASCS Qos preference parameters detail.
 */
typedef struct {
    //uint8_t  sdu_interval_min[SDU_INTERVAL_SIZE];
    //uint8_t  sdu_interval_max[SDU_INTERVAL_SIZE];
    //uint8_t  preferred_framing;
    //uint8_t  preferred_phy;
    //uint16_t preferred_max_sdu;
    uint8_t  preferred_retranmission_num;
    uint16_t max_transport_latency;
    uint32_t supported_presentation_delay_min: 24;
    uint32_t supported_presentation_delay_max: 24;
    uint32_t preferred_presentation_delay_min: 24;
    uint32_t preferred_presentation_delay_max: 24;
} PACKED ble_ascs_default_qos_parameters_t;

/**
 * @}
 */


/**
 * @brief                       This function distributes ASCS request to corresponded handler from specified remote device.
 * @param[in] req               is the type of ASCS request.
 * @param[in] handle            is the connection handle of the Bluetooth link.
 * @param[in] charc_idx         is the characteristic index.
 * @param[in] data              is the data.
 * @param[in] size              is the size of data.
 * @param[in] offset            is the offset.
 * @return                      the size of responded data.
 *
 */
uint32_t ble_ascs_gatt_request_handler(ble_ascs_gatt_request_t req, bt_handle_t handle, uint8_t charc_idx, void *data, uint16_t size, uint16_t offset);

/**
 * @}
 * @}
 */

#endif  /* __BLE_ASCS_DEF_H__ */

