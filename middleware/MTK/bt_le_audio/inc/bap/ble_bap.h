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



#ifndef __BLE_BAP_H__
#define __BLE_BAP_H__

/**
 * @addtogroup Bluetooth
 * @{
 * @addtogroup Bluetooth BLE BAP
 * @{
 * This section introduces the BAP operation codes, request and response structures, API prototypes.
 *
 * Terms and Acronyms
 * ======
 * |Terms                         |Details                                                                  |
 * |------------------------------|-------------------------------------------------------------------------|
 * |\b BAP                        | Basic Audio Profile. |
 * |\b ASE                        | Audio Stream Endpoint. |
 * |\b BASE                       | Broadcast Audio Stream Endpoint. |
 * |\b CIS                        | Connected Isochronous Stream. |
 * |\b BIS                        | Broadcast Isochronous Stream. |
 * |\b BIG                        | Broadcast Isochronous Group. |
 * |\b EA                         | Extended Advertising. |
 * |\b PA                         | Periodic Advertising. |
 */

#ifndef PACKED
#define PACKED  __attribute__((packed))
#endif

#include "bt_le_audio_def.h"
#include "ble_ascs_def.h"
#include "bt_system.h"
#include "ble_ascs.h"
#include "bt_gap_le_audio.h"

/**
 * @defgroup Bluetoothbt_BAP_define Define
 * @{
 * This section defines the maximum number used in BAP.
 */

#define MAX_ASE_NUM                             5       /**< The maximum number of ASE instances. */
#define MAX_CIS_NUM                             2       /**< The maximum number of CIS connection. */
#define MAX_BIS_NUM                             2       /**< The maximum number of BIS connection. */
#define MAX_BIG_NUM                             1       /**< The maximum number of BIG to sync. */
#define MAX_PA_NUM                              2       /**< The maximum number of Periodic Advertising. */
#define MAX_BIG_SUBGROUP_NUM                    4       /**< The maximum number of subgroups used to group BISes present in the BIG. */
#define MAX_BIS_NUM_IN_SUBGROUP                 4       /**< The maximum number of BIS in the subgroup. */
#define MAX_BROADCAST_AUDIO_ANNOUNCEMENTS_NUM   10      /**< The maximum number of Broadcast Audio Announcements. */
#define MAX_PA_SYNC_RETRY_NUM                   1       /**< The maximum number of PA sync retry. */

#define DEFAULT_SCAN_TIMEOUT        (3000)              /**< The default scan duration.(uint:10ms) */

/**
 * @defgroup Bluetoothbt_BAP_define Define
 * @{
 * This section defines the UUID used in BAP.
 */
#define BT_BAP_UUID16_BROADCAST_AUDIO_ANNOUNCEMENTS_SERVICE      0x1852     /**< Broadcast Audio Announcement Service UUID. */
#define BT_BAP_UUID16_BASIC_AUDIO_ANNOUNCEMENTS_SERVICE          0x1851     /**< Basic Audio Announcement Service UUID. */

typedef void (*ble_bap_callback_t)(uint8_t event_id, void *p_msg);


/**
 * @brief The BAP event report to user
 */
enum {
    /*Unicast*/
    BLE_BAP_ASE_STATE_NOTIFY,                               /**< The notification when ASE state changes, with #bap_ase_state_notify_t as the payload in the callback function.*/
    BLE_BAP_ASE_CODEC_CONFIG_IND,                           /**< An ASE Config Codec operation initiated by BAP client, with #bap_ase_codec_config_ind_t as the payload in the callback function.*/
    BLE_BAP_ASE_QOS_CONFIG_IND,                             /**< An ASE Config Qos operation initiated by BAP client, with #bap_ase_qos_config_ind_t as the payload in the callback function.*/
    BLE_BAP_ASE_ENABLE_IND,                                 /**< An ASE Enable operation initiated by BAP client, with #bap_ase_enable_ind_t as the payload in the callback function.*/
    BLE_BAP_CIS_REQUEST_IND,                                /**< The CIS request initiated by BAP client, with #bap_cis_established_ind_t as the payload in the callback function.*/
    BLE_BAP_CIS_ESTABLISHED_NOTIFY,                         /**< The notification when CIS is established, with #bap_cis_established_notify_t as the payload in the callback function.*/
    BLE_BAP_ASE_DISABLE_IND,                                /**< An ASE Disable operation initiated by BAP client, with #bap_ase_disable_ind_t as the payload in the callback function.*/
    BLE_BAP_CIS_DISCONNECTED_NOTIFY,                        /**< The notification when CIS is disconnected, with #bap_cis_disconnected_notify_t as the payload in the callback function.*/
    BLE_BAP_ASE_RELEASE_IND,                                /**< An ASE Release operation initiated by BAP client, with #bap_ase_release_ind_t as the payload in the callback function.*/
    BLE_BAP_ASE_RECEIVER_START_READY_IND,                   /**< An ASE Receiver Start Ready operation initiated by BAP client, with #bap_ase_receiver_start_ready_ind_t as the payload in the callback function.*/
    BLE_BAP_ASE_RECEIVER_STOP_READY_IND,                    /**< An ASE Receiver Stop Ready operation initiated by BAP client, with #bap_ase_receiver_start_ready_ind_t as the payload in the callback function.*/
    BLE_BAP_ASE_UPDATE_METADATA_IND,                        /**< An ASE Update metadata operation initiated by BAP client, with #bap_ase_update_metadata_ind_t as the payload in the callback function.*/

    /*Broadcast*/ /*12*/
    BLE_BAP_BASE_BROADCAST_AUDIO_ANNOUNCEMENTS_IND,         /**< An BASE Broadcast Audio Announcements received in Extended Advertising, with #bap_broadcast_audio_announcements_ind_t as the payload in the callback function.*/
    BLE_BAP_BASE_PERIODIC_ADV_SYNC_ESTABLISHED_NOTIFY,      /**< The notification when BASE Periodic Advertising Synchronization is established, with #bap_periodic_adv_sync_established_notify_t as the payload in the callback function.*/
    BLE_BAP_BASE_BASIC_AUDIO_ANNOUNCEMENTS_IND,             /**< An BASE Basic Audio Announcements received in Periodic Advertising, with #bap_basic_audio_announcements_ind_t as the payload in the callback function.*/
    BLE_BAP_BASE_BIGINFO_ADV_REPORT_IND,
    BLE_BAP_BASE_BIG_SYNC_IND,
    BLE_BAP_BASE_BIG_SYNC_ESTABLISHED_NOTIFY,
    BLE_BAP_BASE_PERIODIC_ADV_TERNIMATE_IND,
    BLE_BAP_BASE_BIG_TERNIMATE_IND,
    BLE_BAP_BASE_SCAN_TIMEOUT_IND,
    BLE_BAP_BASE_BASS_ADD_SOURCE_IND,
    BLE_BAP_BASE_SCAN_STOPPPED_IND,
    BLE_BAP_BASE_BASS_MODIFY_SOURCE_IND,

    BLE_BAP_BASE_PERIODIC_ADV_TERMINATE_CNF,
    BLE_BAP_BASE_BIG_TERMINATE_CNF,

    BLE_BAP_SETUP_ISO_DATA_PATH_NOTIFY,
    BLE_BAP_REMOVE_ISO_DATA_PATH_NOTIFY,
};

/**
 * @defgroup Bluetoothbt_BAP_define Struct
 * @{
 * This section defines basic data structures for the BAP.
 */

/**
 *  @brief This structure defines the parameter data type used in #bap_basic_audio_announcements_level_2_t.
 */
typedef struct {
    uint8_t bis_index;                                      /**< BIS_index value for the BIS in subgroup. */
    uint8_t codec_specific_configuration_length;            /**< Length of the Codec_Specific_Configuration for the BIS in subgroup */
    uint8_t *codec_specific_configuration;                  /**< Codec-specific configuration parameters for the BIS in subgroup */
}  ble_bap_basic_audio_announcements_level_3_t;

/**
 *  @brief This structure defines the parameter data type used in #bap_basic_audio_announcements_level_1_t.
 */
typedef struct {
    uint8_t num_bis;                                        /**< Number of BIS in the subgroup */
    uint8_t codec_id[AUDIO_CODEC_ID_SIZE];                  /**< The codec ID. */
    uint8_t codec_specific_configuration_length;            /**< Length of the Codec_Specific_Configuration for Level 2*/
    uint8_t *codec_specific_configuration;                  /**< Codec-specific configuration parameters for the subgroup */
    uint8_t metadata_length;                                /**< Length of the Metadata for the subgroup */
    uint8_t *metadata;                                      /**< Series of LTV structures containing Metadata for the subgroup */
    ble_bap_basic_audio_announcements_level_3_t *level_3;       /**< Basic audio announcements parameters for Level 3 */
} ble_bap_basic_audio_announcements_level_2_t;

/**
 *  @brief This structure defines the parameter data type used in #bap_basic_audio_announcements_t and #bap_basic_audio_announcements_ind_t.
 */
typedef struct {
    uint8_t presentation_delay[PRESENTATION_DELAY_SIZE];        /**< The presentation delay. */
    uint8_t num_subgroups;                                      /**< Number of subgroups used to group BISes present in the BIG. */
    ble_bap_basic_audio_announcements_level_2_t *level_2;       /**< Basic audio announcements parameters for Level 2 */
} ble_bap_basic_audio_announcements_level_1_t;

/**
 *  @brief This structure defines the data type of Basic Audio Announcements.
 */
typedef struct {
    uint8_t length;                                             /**< Length of Basic audio announcements. */
    uint8_t *raw_data;                                          /**< Raw data of Basic audio announcements. */
    ble_bap_basic_audio_announcements_level_1_t level_1;            /**< Basic audio announcements parameters for Level 1 */
} ble_bap_basic_audio_announcements_t;

/**
 *  @brief This structure defines the data type of BIG recorded in decive.
 */
typedef struct {
    bool is_sync;                                               /**< Whether sync to BIG or not */
    uint8_t big_handle;                                         /**< Used to identify the BIG. */
    uint8_t num_bis;                                            /**< Total number of BISes to synchronize. */
    bt_handle_t bis_handle[MAX_BIS_NUM];
    uint8_t framing;
    uint16_t iso_interval;
    uint16_t max_pdu;
    uint16_t max_sdu;
    uint8_t sdu_interval[3];
    uint8_t phy;
    uint8_t encryption;
    uint16_t data_path_handle;                                  /**< Next BIS handle to set/remove iso data path. */
} ble_bap_big_info_t;

/**
 *  @brief This structure defines the data type of PA recorded in decive.
 */
typedef struct {
    bt_handle_t sync_handle;                                    /**< The sync handle of the periodic advertising. */
    bool is_sync;                                               /**< Whether sync to periodic advertising or not */
    uint8_t advertising_sid;                                    /**< The advertising SID. */
    bt_addr_t advertiser_addr;                                  /**< The advertiser's address. */
    uint32_t broadcast_id;                                   /**< The Broadcast ID. */
    ble_bap_basic_audio_announcements_t pa_data;                    /**< Periodic advertising data. */
    ble_bap_big_info_t big_info;
} ble_bap_pa_info_t;

typedef struct {
    //uint8_t direction;
    uint8_t codec[AUDIO_CODEC_ID_SIZE];
    uint8_t sampling_frequency;
    uint8_t sdu_interval[SDU_INTERVAL_SIZE];
    //uint8_t framing;
    //uint8_t phy;
    uint8_t presentation_delay[PRESENTATION_DELAY_SIZE];  /* MTK_David: Enable for AM */
    //uint8_t retransmission_number;
    uint16_t frame_payload_length;
    //uint16_t transport_latency;
    bool is_stereo;
    uint8_t metadata_length;
    uint8_t *metadata;
} PACKED ble_bap_stream_config_info_t;

/**
 *  @brief This structure defines the parameter data type for event #BAP_ASE_STATE_NOTIFY.
 */
typedef struct {
    bt_handle_t connect_handle;                                 /**< Connection handle. */
    uint8_t ase_id;                                             /**< The ASE_ID for this ASE. */
    uint8_t ase_state;                                          /**< The state for this ASE. */
} ble_bap_ase_state_notify_t;

/**
 *  @brief This structure defines the parameter data type of response code used in ASE Control operations.
 */
typedef struct {
    uint8_t response_code;                                      /**< ASE Control Point response code. */
    uint8_t reason;                                             /**< ASE Control Point error reason. */
} ble_bap_ase_response_reason_t;

/**
 *  @brief This structure defines the parameter data type for event #BAP_ASE_CODEC_CONFIG_IND.
 */
typedef struct {
    bt_handle_t connect_handle;                                 /**< Connection handle. */
    ble_ascs_config_codec_operation_ind_t *codec_config;         /**< Codec configuration for this ASE. */
} PACKED ble_bap_ase_codec_config_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BAP_ASE_QOS_CONFIG_IND.
 */
typedef struct {
    bt_handle_t connect_handle;                                 /**< Connection handle. */
    ble_ascs_config_qos_operation_t *qos_config;                 /**< QoS configuration for this ASE. */
} PACKED ble_bap_ase_qos_config_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BAP_ASE_ENABLE_IND.
 */
typedef struct {
    bt_handle_t connect_handle;                                 /**< Connection handle. */
    bool is_last_ase;                                           /**< Used to check this ASE is the last one for the operation . */
    uint8_t  ase_id;                                            /**< The ASE_ID for this ASE. */
    uint8_t  metadata_length;                                   /**< Length of the Metadata parameter for this ASE. */
    uint8_t  metadata[1];                                       /**< LTV-formatted Metadata for this ASE. */
} PACKED ble_bap_ase_enable_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BAP_ASE_DISABLE_IND, #BAP_ASE_RELEASE_IND, #BAP_ASE_RECEIVER_START_READY_IND and #BAP_ASE_RECEIVER_STOP_READY_IND.
 */
typedef struct {
    bt_handle_t connect_handle;                                 /**< Connection handle. */
    uint8_t  ase_id;                                            /**< The ASE_ID for this ASE. */
} PACKED ble_bap_ase_disable_ind_t, ble_bap_ase_release_ind_t, ble_bap_ase_receiver_start_ready_ind_t, ble_bap_ase_receiver_stop_ready_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BAP_ASE_UPDATE_METADATA_IND.
 */
typedef struct {
    bt_handle_t connect_handle;                                 /**< Connection handle. */
    uint8_t  ase_id;                                            /**< The ASE_ID for this ASE. */
    uint8_t  metadata_length;                                   /**< Length of the Metadata parameter for this ASE. */
    uint8_t  metadata[1];                                       /**< LTV-formatted Metadata for this ASE. */
} PACKED ble_bap_ase_update_metadata_ind_t;

/**
 *  @brief This structure defines the data type of ASE ID list when multiple ASEs are being enable.
 */
typedef struct {
    uint8_t num_of_ase;                                         /**< Total number of ASE_IDs used in this list. */
    uint8_t ase_id[3];                                          /**< The ASE ID list. */
} PACKED ble_bap_ase_id_list_t;

/**
 *  @brief This structure defines the data type of CIS established event.
 */
typedef struct
{
    uint8_t           status;                           /**< The status of the CIS. */
    bt_handle_t       connection_handle;                /**< Connection handle of the CIS. */
    uint32_t          cig_sync_delay : 24;              /**< The maximum time, in microseconds, for transmission of PDUs of all CISes in a CIG in an isochronous interval. */
    uint32_t          cis_sync_delay : 24;              /**< The maximum time, in microseconds, for transmission of PDUs of a CIS in an isochronous interval. */
    uint32_t          transport_latency_m_to_s : 24;    /**< The maximum time, in microseconds, for transmission of SDUs of all CISes in a CIG from slave to master. */
    uint32_t          transport_latency_s_to_m : 24;    /**< The maximum time, in microseconds, for transmission of SDUs of all CISes in a CIG from master to slave. */
    uint8_t           phy_m_to_s;                       /**< The transmitter PHY of packets used from the slave to master. */
    uint8_t           phy_s_to_m;                       /**< The transmitter PHY of packets used from the slave to master. */
    uint8_t           nse; /**< The maximum number of subevents in each isochronous event. */
    uint8_t           bn_m_to_s; /**< The burst number for master to slave transmission. */
    uint8_t           bn_s_to_m; /**< The burst number for slave to master transmission. */
    uint8_t           ft_m_to_s; /**< The flush timeout, in multiples of the ISO_Interval for the CIS, for each payload sent from the master to the slave. */
    uint8_t           ft_s_to_m; /**< The flush timeout, in multiples of the ISO_Interval for the CIS, for each payload sent from the slave to the master. */
    uint16_t          max_pdu_m_to_s; /**< Maximum size, in octets, of the payload from master to slave. */
    uint16_t          max_pdu_s_to_m; /**< Maximum size, in octets, of the payload from slave to master. */
    uint16_t          iso_interval; /**< The time between two consecutive CIS anchor points. Range: 0x0004 to 0x0C80, Time = N * 1.25 ms. */

}PACKED ble_bap_cis_established_data_t;

/**
 *  @brief This structure defines the parameter data type for event #BAP_CIS_REQUEST_IND, #BAP_CIS_ESTABLISHED_NOTIFY and #BAP_CIS_DISCONNECTED_NOTIFY.
 */
typedef struct {
    bt_handle_t connect_handle;                                 /**< Connection handle. */
    bt_handle_t cis_handle;                                     /**< CIS handle. */
    ble_bap_ase_id_list_t ase_id_list;                              /**< The ASE list for this CIS handle. */
    uint16_t iso_interval;                                      /**< ISO interval. */
} PACKED ble_bap_cis_established_ind_t, ble_bap_cis_established_notify_t, ble_bap_cis_disconnected_notify_t;

/**
 *  @brief This structure defines the parameter data type in function #ble_bap_codec_config_autonomously().
 */
typedef struct {
    uint8_t ase_id;                                              /**< The ASE_ID for this ASE. */
    //uint8_t direction;                                           /**< Direction of this ASE with respect to the server (Audio Source or Audio Sink). */
    uint8_t target_latency;                                      /**< The target latency. */
    uint8_t target_phy;                                          /**< PHY parameter target to achieve the Target_Latency value. */
    uint8_t codec_id[AUDIO_CODEC_ID_SIZE];                       /**< The codec id. */
    uint8_t codec_specific_configuration_length;                 /**< Length of the Codec_Specific_Configuration value for this ASE. */
    uint8_t codec_specific_configuration[1];                     /**< Codec-Specific Configuration for this ASE. */
} PACKED ble_bap_config_codec_operation_t;

/**
 *  @brief This structure defines the data type of LTV structure.
 */
typedef struct {
    uint8_t length;                                              /**< Length for this LTV structure. */
    uint8_t type;                                                /**< Type for this LTV structure. */
    uint8_t value[1];                                            /**< Value for this LTV structure. */
} PACKED ble_bap_ltv_structure_t;

/**
 *  @brief This structure defines the parameter data type for event #BAP_BASE_BROADCAST_AUDIO_ANNOUNCEMENTS_IND.
 */
typedef struct {
    bt_addr_t addr;                                             /**< The address for broadcast source device. */
    uint8_t advertising_sid;                                    /**< The advertising SID. */
    uint8_t data_length;                                        /**< The data length for broadcast audio announcements. */
    uint8_t *data;                                              /**< The data for broadcast audio announcements. */
    int8_t  rssi;                                               /**< The RSSI. */
} PACKED ble_bap_broadcast_audio_announcements_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BAP_BASE_PERIODIC_ADV_SYNC_ESTABLISHED_NOTIFY.
 */
typedef struct {
    bt_status_t status;
    bt_handle_t sync_handle;                                    /**< The sync handle of the periodic advertising. */
    bt_addr_t advertiser_addr;                               /**< The advertiser's address. */
    uint16_t pa_interval;                                       /**< Periodic advertising interval. */
    uint8_t advertising_sid;                                    /**< Advertising SID. */
} ble_bap_periodic_adv_sync_established_notify_t;

/**
 *  @brief This structure defines the parameter data type for event #BAP_BASE_BASIC_AUDIO_ANNOUNCEMENTS_IND.
 */
typedef struct {
    bt_handle_t sync_handle;                                    /**< The sync handle of the periodic advertising. */
    ble_bap_basic_audio_announcements_level_1_t *level_1;            /**< Basic audio announcements parameters for Level 1 */
} PACKED ble_bap_basic_audio_announcements_ind_t;

/**
 *  @brief This structure defines the parameter data type for event #BT_GAP_LE_BIGINFO_ADV_REPORT_IND.
 */
typedef struct {
    bt_handle_t sync_handle;                                    /**< The sync handle of the periodic advertising. */
    uint8_t num_bis;                                            /**< Number of BIS in BIG */
    uint8_t sdu_interval[SDU_INTERVAL_SIZE];                    /**< SDU_Interval for this BIG. */
    uint16_t maximum_sdu_size;                                  /**< Maximum_SDU for this BIG. */
    uint8_t phy;                                                /**< PHY for this BIG. */
    uint8_t framing;                                            /**< Framing for this BIG. */
} ble_bap_big_info_adv_report_ind_t;

typedef struct {
    bt_handle_t sync_handle;                                    /**< The sync handle of the periodic advertising. */
    uint8_t big_handle;                                         /**< The big handle of the BIG. */
    uint8_t num_bis;                                            /**< Number of BIS in BIG */
    uint8_t bis_indices[1];
} ble_bap_big_sync_ind_t;

typedef struct {
    bt_status_t status;
    uint8_t big_handle;
    uint8_t num_bis;
    bt_handle_t connection_handle_list[1];
} ble_bap_big_sync_established_notify_t;

typedef struct {
    bt_handle_t sync_handle;
} ble_bap_periodic_adv_terminate_ind_t;

typedef struct {
    bt_handle_t big_handle;
} ble_bap_big_terminate_ind_t;

typedef struct {
    uint8_t advertising_addr_type;
    bt_bd_addr_t advertiser_addr;
    uint8_t advertising_sid;
    uint32_t broadcast_id: 24;
    uint8_t pa_sync;
    uint16_t pa_interval;
    uint32_t bis_sync;
    uint8_t subgroup;
} PACKED ble_bap_bass_add_source_param_t, ble_bap_bass_modify_source_param_t;

typedef struct {
    bt_handle_t connect_handle;
    uint8_t source_id;
    ble_bap_bass_add_source_param_t *param;
} ble_bap_bass_add_source_ind_t;

typedef struct {
    bt_status_t status;
} ble_bap_scan_stopped_ind_t;

typedef struct {
    bt_handle_t connect_handle;
    uint8_t source_id;
    ble_bap_bass_modify_source_param_t *param;
} ble_bap_bass_modify_source_ind_t;

typedef bt_gap_le_setup_iso_data_path_cnf_t ble_bap_set_setup_iso_data_path_notify_t;
typedef bt_gap_le_remove_iso_data_path_cnf_t ble_bap_set_remove_iso_data_path_notify_t;
/**
 * @}
 */


BT_EXTERN_C_BEGIN

/**
 * @brief                       This function is a BAP initialization API.
 * @param[in] callback          is callback function used to handle BAP events.
 * @param[in] max_link_num      is maximum number of BLE link.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_init(ble_bap_callback_t callback, uint8_t max_link_num);

/**
 * @brief                       This function is a BAP deinitialization API.
 * @param[in] callback          is callback function used to handle BAP events.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_deinit(ble_bap_callback_t callback);

/**
 * @brief                       This function responds to the codec config operation from the specified remote device.
 * @param[in] connection_handle is the connection handle of the Bluetooth link.
 * @param[in] ase_id            is the ASE ID.
 * @param[in] response_code     is the response code.
 * @param[in] error_response    is error reason.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_codec_config_response(bt_handle_t connect_handle, uint8_t ase_id, uint8_t response_code, uint8_t error_response);

/**
 * @brief                       This function responds to the ASE qos config operation from the specified remote device.
 * @param[in] connection_handle is the connection handle of the Bluetooth link.
 * @param[in] ase_id            is the ASE ID.
 * @param[in] response_code     is the response code.
 * @param[in] error_response    is error reason.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_ase_qos_config_response(bt_handle_t connect_handle, uint8_t ase_id, uint8_t response_code, uint8_t error_response);

/**
 * @brief                       This function responds to the ASE enable operation from the specified remote device.
 * @param[in] connection_handle is the connection handle of the Bluetooth link.
 * @param[in] ase_id            is the ASE ID.
 * @param[in] response_code     is the response code.
 * @param[in] error_response    is error reason.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_ase_enable_response(bt_handle_t connect_handle, uint8_t ase_id, uint8_t response_code, uint8_t error_response);

/**
 * @brief                       This function responds to the ASE enable receiver start/stop ready operation from the specified remote device.
 * @param[in] connection_handle is the connection handle of the Bluetooth link.
 * @param[in] ase_id            is the ASE ID.
 * @param[in] response_code     is the response code.
 * @param[in] error_response    is error reason.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_ase_receiver_start_stop_ready_response(bt_handle_t connect_handle, uint8_t ase_id, uint8_t response_code, uint8_t error_response);

/**
 * @brief                       This function responds to the ASE disable operation from the specified remote device.
 * @param[in] connection_handle is the connection handle of the Bluetooth link.
 * @param[in] ase_id            is the ASE ID.
 * @param[in] response_code     is the response code.
 * @param[in] error_response    is error reason.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_ase_disable_response(bt_handle_t connect_handle, uint8_t ase_id, uint8_t response_code, uint8_t error_response);

/**
 * @brief                       This function responds to the ASE release operation from the specified remote device.
 * @param[in] connection_handle is the connection handle of the Bluetooth link.
 * @param[in] ase_id            is the ASE ID.
 * @param[in] response_code     is the response code.
 * @param[in] error_response    is error reason.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_ase_release_response(bt_handle_t connect_handle, uint8_t ase_id, ble_bap_ase_response_reason_t response);

/**
 * @brief                       This function responds to the ASE update metadata operation from the specified remote device.
 * @param[in] connection_handle is the connection handle of the Bluetooth link.
 * @param[in] ase_id            is the ASE ID.
 * @param[in] response_code     is the response code.
 * @param[in] error_response    is error reason.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_ase_update_metadata_response(bt_handle_t connect_handle, uint8_t ase_id, uint8_t response_code, uint8_t error_response);

/**
 * @brief                       This function autonomously initiates codec config operation.
 * @param[in] connection_handle is the connection handle of the Bluetooth link.
 * @param[in] codec_config      is the codec configuration.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_codec_config_autonomously(bt_handle_t connect_handle, ble_bap_config_codec_operation_t *codec_config);

/**
 * @brief                       This function autonomously initiates update metadata operation.
 * @param[in] connection_handle is the connection handle of the Bluetooth link.
 * @param[in] parm              is the parameters of metadata.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_update_metadata_autonomously(bt_handle_t connect_handle, ble_ascs_update_metadata_operation_t *parm);

/**
 * @brief                       This function responds to CIS request from the specified remote device.
 * @param[in] connection_handle is the connection handle of the CIS link.
 * @param[in] is_accept         is the codec configuration.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_cis_request_response(bt_handle_t cis_handle, bool is_accept);

/**
 * @brief                       This function disconnects CIS connection from the specified remote device.
 * @param[in] connection_handle is the connection handle of the CIS link.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_disconnect_cis(bt_handle_t cis_handle);

/**
 * @brief                           This function sets CIS data path to controller.
 * @param[in] cis_num               is the number of CIS link.
 * @param[in] cis_handle            is the connection handle of the CIS link list.
 * @param[in] path_id               is the ID of the data path list.
 * @return                          #BT_STATUS_SUCCESS, the operation completed successfully.
 *                                  #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_set_cis_data_path(uint8_t cis_num, bt_handle_t *cis_handle, uint8_t *path_id);

/**
 * @brief                           This function sets CIS data path to controller.
 * @param[in] big_handle            is the connection handle of the BIG.
 * @return                          #BT_STATUS_SUCCESS, the operation completed successfully.
 *                                  #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_set_bis_data_path(uint8_t big_handle);

/**
 * @brief                           This function removes CIS data path to controller.
 * @param[in] cis_num               is the number of CIS link.
 * @param[in] cis_handle            is the connection handle of the CIS link list.
 * @param[in] data_path_direction   is the direction of data path list.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_remove_cis_data_path(uint8_t cis_num, bt_handle_t *cis_handle, uint8_t *data_path_direction);


/**
 * @brief                           This function gets LE connection handle by CIS connection handle.
 * @param[in] cis_handle            is the connection handle of the CIS link.
 * @return                          #BLE connection hanmdle.
 */
bt_handle_t ble_bap_get_le_handle_by_cis_handle(bt_handle_t cis_handle);

/**
 * @brief                           This function remove BIS data path to controller.
 * @param[in] big_handle            is the connection handle of the BIG link.
 * @param[in] data_path_direction   is the direction of data path.
 * @return                          #BT_STATUS_SUCCESS, the operation completed successfully.
 *                                  #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_remove_bis_data_path(uint8_t big_handle, uint8_t data_path_direction);

/**
 * @brief                       This function sets the broadcast source address in the white list.
 * @param[in] op                is the option to clear, add, or remove from white list.
 * @param[in] addr_type         is the address tyep.
 * @param[in] addr              is the address to clear, add, or remove.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bt_status_t ble_bap_set_white_list_ex(bt_gap_le_set_white_list_op_t op, uint8_t addr_type, uint8_t *p_addr);

/**
 * @deprecated Please use ble_bap_set_white_list_ex instead.
 * @brief                       This function sets the broadcast source address in the white list.
 * @param[in] op                is the option to clear, add, or remove from white list.
 * @param[in] addr              is the address to clear, add, or remove.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bt_status_t ble_bap_set_white_list(bt_gap_le_set_white_list_op_t op, uint8_t *p_addr);

/**
 * @brief                       This function starts scanning broadcast source for broadcast streaming.
 * @param[in] duration          is the scanning duration.
 * @param[in] scan_type         is the scanning type, accept all adv or scan adv in white list.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bt_status_t ble_bap_scan_broadcast_source(uint16_t duration, bt_hci_scan_filter_type_t scan_type);

/**
 * @brief                       This function stops scanning broadcast source for broadcast streaming.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bt_status_t ble_bap_stop_scanning_broadcast_source(void);

/**
 * @brief                       This function syncs broadcast source with periodic advertising.
 * @param[in] address           is the Bluetooth address of a remote device.
 * @param[in] advertising_sid   is the Advertising SID subfield in the ADI field used to identify the Periodic Advertising.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bt_status_t ble_bap_sync_broadcast_source_with_periodic_advertising(bt_addr_t addr, uint8_t advertising_sid);

/**
 * @brief                       This function gets broadcast id from pa info.
 * @param[in] advertiser_addr   is the address of the periodic advertising.
 * @return                      #broadcast id.
 */
uint32_t ble_bap_get_broadcast_id_by_addr(bt_addr_t addr);

/**
 * @brief                       This function gets basic audio announcements level_1 data.
 * @param[in] sync_handle       is the sync handle of the periodic advertising.
 * @return                      #data point to basic audio announcements level_1.
 */
ble_bap_basic_audio_announcements_level_1_t *ble_bap_get_basic_audio_announcements_level_1(bt_handle_t sync_handle);

/**
 * @brief                       This function checks whether bis indices are valid or not.
 * @param[in] sync_handle       is the sync handle of the periodic advertising.
 * @param[in] num_bis           is the number of bises.
 * @param[in] bis_indices       is the bis index list.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_is_bis_valid(bt_handle_t sync_handle, uint8_t num_bis, uint8_t *bis_indices);

/**
 * @brief                       This function starts receiving BIS audio data with the specified remote device.
 * @param[in] sync_handle       is the sync handle of the periodic advertising.
 * @param[in] num_bis           is the number of bises to sync.
 * @param[in] bis_indices       is the bises list.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bt_status_t ble_bap_start_broadcast_reception(bt_handle_t sync_handle, uint8_t big_handle, uint8_t num_bis, uint8_t *bis_indices);

/**
 * @brief                       This function stops syncing broadcast source with broadcast with periodic advertising.
 * @param[in] sync_handle       is the sync handle of the periodic advertising.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bt_status_t ble_bap_stop_syncing_to_periodic_advertising(bt_handle_t sync_handle);

/**
 * @brief                       This function stops receiving BIS audio data from the specified remote device.
 * @param[in] big_handle        is the big handle of the BIS.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bt_status_t ble_bap_stop_broadcast_reception(uint8_t big_handle);

/**
 * @brief                       This function resets broadcast state.
 * @param[in] sync_handle       is the sync handle of the periodic advertising.
 * @return                      None.
 */
void ble_bap_reset_broadcast_state(bt_handle_t sync_handle);

/**
 * @brief                       This function gets the broadcast configuration information.
 * @param[in] big_handle        is the big handle of the BIS.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_clear_big_info(bt_handle_t big_handle);

/**
 * @brief                       This function gets the broadcast configuration information.
 * @param[in] sync_handle       is the sync handle of the periodic advertising.
 * @param[out] config_info      is the configuration information of the broadcast stream.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_get_broadcast_config_info(bt_handle_t sync_handle, ble_bap_stream_config_info_t *config_info);

/**
 * @brief                       This function configures which BIG/BIS to synchronize to.
 * @param[in] big_handle        is the big handle of the BIG.
 * @param[in] num_bis           is the number of the BIS to synchronize to.
 * @param[in] bis_indices       is the big indices of the BISes.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bt_status_t ble_bap_config_sync_big(uint8_t big_handle, uint8_t num_bis, uint8_t *bis_indices);

/**
 * @brief                       This function sets the broadcast encryption.
 * @param[in] enctryption       is encrptyed or not.
 * @return                      #none
 */
void ble_bap_set_broadcast_encryption(bool encryption);

/**
 * @brief                       This function sets the broadcast code.
 * @param[in] broadcast_code    is the broadcast code of the BIG stream.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bt_status_t ble_bap_set_broadcast_code(uint8_t *broadcast_code);

/**
 * @brief                       This function gets the broadcast code.
 * @param[in] broadcast_code    is the broadcast code of the BIG stream.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bt_status_t ble_bap_get_broadcast_code(uint8_t *broadcast_code);

/**
 * @brief                       This function resets the big info.
 * @param[in] sync_handle       is the sync handle of the periodic advertising.
 * @return                      #true, the operation completed successfully.
 *                              #false, the operation has failed.
 */
bool ble_bap_reset_big_info(bt_handle_t sync_handle);

/**
 * @brief                       This function responds to the BASS add source operation from the specified remote device.
 * @param[in] connection_handle is the connection handle of the Bluetooth link.
 * @param[in] source_id         is the source id.
 * @param[in] is_accept         is accept or not.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_bass_add_source_response(bt_handle_t connect_handle, uint8_t source_id, bool is_accept);

/**
 * @brief                       This function returns whether the scanning is in progress.
 * @return                      #true, the scanning is in progress.
 *                              #false, the scanning is not in progress.
 */
bool ble_bap_is_scanning_broadcast_source(void);

/**
 * @brief                       This function returns whether the pa is syncing or synced.
 * @return                      #true, the pa syncing or synced is in progress.
 *                              #false, the pa syncing or synced is not in progress.
 */
bool ble_bap_is_syncing_to_pa(void);

/**
 * @brief                       This function responds to the BASS modify source operation from the specified remote device.
 * @param[in] connection_handle is the connection handle of the Bluetooth link.
 * @param[in] source_id         is the source id.
 * @param[in] is_accept         is accept or not.
 * @return                      #BT_STATUS_SUCCESS, the operation completed successfully.
 *                              #BT_STATUS_FAIL, the operation has failed.
 */
bt_status_t ble_bap_bass_modify_source_response(bt_handle_t connect_handle, uint8_t source_id, bool is_accept);

/**
 * @brief                       This function set bis index, to ensure the BIS can be established.
 * @param[in] bis_sync_state    is the BIS index to be established.
 * @return                      NONE.
 */
void ble_bap_config_bis_sync_state(uint32_t bis_sync_state);

/**
 * @brief                       This function send BASS notify to all clients.
 * @return                      #Number, Success number of client to be notified.
 */
uint8_t ble_bap_bass_notify_all_clients(void);

BT_EXTERN_C_END
/**
 * @}
 * @}
 */

#endif  /* __BT_BAP_H__*/

