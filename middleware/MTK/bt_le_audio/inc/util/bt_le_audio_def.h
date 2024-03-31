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

#ifndef __BT_LE_AUDIO_DEF_H__
#define __BT_LE_AUDIO_DEF_H__

#include "ble_tbs_def.h"
#include "ble_mcs_def.h"
#include "ble_vcs_def.h"
#include "ble_aics_def.h"
#include "ble_mics_def.h"
#include "ble_tmas_def.h"

/**
 * @brief Defines for call state, refer to #ble_tbs_state_t.
 */
#define bt_le_audio_call_state_t                    ble_tbs_state_t

/**
 * @brief Defines for call index, refer to #ble_tbs_call_index_t.
 */
#define bt_le_audio_call_index_t                    ble_tbs_call_index_t

/**
 * @brief Defines for call termination reason, refer to #ble_tbs_termination_reason_t.
 */
#define bt_le_audio_call_termination_reason_type_t  ble_tbs_termination_reason_t

/**
 * @brief Defines for LE AUDIO UCI.
 */
enum {
    BT_LE_AUDIO_UCI_SKYPE,  /**< Skype.*/
    BT_LE_AUDIO_UCI_LYNC,   /**< Lync.*/
    BT_LE_AUDIO_UCI_FACETIME,   /**< Facetime.*/
    BT_LE_AUDIO_UCI_JABBER, /**< Jabber.*/
    BT_LE_AUDIO_UCI_BLACKBERRY_VOICE,   /**< Blackberry Voice.*/
    BT_LE_AUDIO_UCI_BLACKBERRY_MESSENGER,   /**< Blackberry Messenger.*/
    BT_LE_AUDIO_UCI_EYEBEAM,    /**< Eyebeam.*/
    BT_LE_AUDIO_UCI_ANY_WEBRTC_ENABLED_BROWSER, /**< Any WebRTC enabled browser.*/
    BT_LE_AUDIO_UCI_QQ, /**< QQ.*/
    BT_LE_AUDIO_UCI_SPIKA,  /**< Spika.*/
    BT_LE_AUDIO_UCI_GOOGLE_TALK,    /**< Google Talk    .*/
    BT_LE_AUDIO_UCI_FACEBOOK_CHAT,  /**< Facebook Chat  .*/
    BT_LE_AUDIO_UCI_TELEGRAM,   /**< Telegram.*/
    BT_LE_AUDIO_UCI_LINE,   /**< Line.*/
    BT_LE_AUDIO_UCI_WHATSAPP,   /**< WhatsApp.*/
    BT_LE_AUDIO_UCI_VIBER,  /**< Viber.*/
    BT_LE_AUDIO_UCI_MESSAGEME,  /**< MessageMe.*/
    BT_LE_AUDIO_UCI_KAKAOTALK,  /**< KakaoTalk.*/
    BT_LE_AUDIO_UCI_WECHAT, /**< WeChat.*/
    BT_LE_AUDIO_UCI_TANGO,  /**< Tango.*/
    BT_LE_AUDIO_UCI_KIK,    /**< KIK.*/
    BT_LE_AUDIO_UCI_NIMBUZZ,    /**< Nimbuzz.*/
    BT_LE_AUDIO_UCI_HGANGOUTS,  /**< Hangouts.*/
    BT_LE_AUDIO_UCI_CHATON, /**< ChatOn.*/
    BT_LE_AUDIO_UCI_MESSANGER,  /**< Messanger.*/
    BT_LE_AUDIO_UCI_CHATSECURE, /**< ChatSecure.*/
    BT_LE_AUDIO_UCI_ICHAT,  /**< iChat.*/
    BT_LE_AUDIO_UCI_ROUNDS, /**< Rounds.*/
    BT_LE_AUDIO_UCI_IMOIM,  /**< imo.im.*/
    BT_LE_AUDIO_UCI_SKYE_QIK,   /**< Skye Qik.*/
    BT_LE_AUDIO_UCI_LIBON,  /**< Libon.*/
    BT_LE_AUDIO_UCI_VONAGE_MOBILE,  /**< ?Vonage Mobile.*/
    BT_LE_AUDIO_UCI_OOVO,   /**< ooVo.*/

    BT_LE_AUDIO_UCI_NUM     /**< Maximum of UCI.*/
};

/**
 * @brief Defines for media state, refer to #ble_mcs_media_state_t.
 */
#define bt_le_audio_media_state_t                   ble_mcs_media_state_t

/**
 * @brief Defines for volume flag, refer to #ble_vcs_volume_flags_t.
 */
#define bt_le_audio_volume_flag_t                   ble_vcs_volume_flags_t
/**
 * @brief Defines for volume input mute state, refer to #ble_aics_mute_state_t.
 */
#define bt_le_audio_volume_input_mute_state_t       ble_aics_mute_state_t

/**
 * @brief Defines for volume input gain mode, refer to #ble_aics_gain_mode_t.
 */
#define bt_le_audio_volume_input_gain_mode_t        ble_aics_gain_mode_t

/**
 * @brief Defines for volume input status, refer to #ble_aics_audio_input_status_t.
 */
#define bt_le_audio_volume_input_status_t           ble_aics_audio_input_status_t

/**
 * @brief Defines for microphone mute state, refer to #ble_mics_mute_state_t.
 */
#define bt_le_audio_microphone_mute_state_t         ble_mics_mute_state_t

/**
 * @brief Defines for LE AUDIO codec id size.
 */
#define AUDIO_CODEC_ID_SIZE         5

/**
 * @brief Defines for audio direction.
 */
#define AUDIO_DIRECTION_SINK        1   /**< Audio direction sink.*/
#define AUDIO_DIRECTION_SOURCE      2   /**< Audio direction source.*/
typedef uint8_t bt_le_audio_direction_t;

/**
 * @brief Defines for audio locations.
 */
#define AUDIO_LOCATION_NONE                     0x00000000  /**< Mono/Unspecified.*/
#define AUDIO_LOCATION_FRONT_LEFT               0x00000001  /**< Front Left.*/
#define AUDIO_LOCATION_FRONT_RIGHT              0x00000002  /**< Front Right.*/
#define AUDIO_LOCATION_FRONT_CENTER             0x00000004  /**< Front Center.*/
#define AUDIO_LOCATION_LOW_FREQ_EFFECT1         0x00000008  /**< Low Frequency Effects 1.*/
#define AUDIO_LOCATION_BACK_LEFT                0x00000010  /**< Back Left.*/
#define AUDIO_LOCATION_BACK_RIGHT               0x00000020  /**< Back Right.*/
#define AUDIO_LOCATION_FRONT_LEFT_OF_CENTER     0x00000040  /**< Front Left of Center.*/
#define AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER    0x00000080  /**< Front Right of Center.*/
#define AUDIO_LOCATION_BACK_CENTER              0x00000100  /**< Back Center.*/
#define AUDIO_LOCATION_LOW_FREQ_EFFECT2         0x00000200  /**< Low Frequency Effects 2.*/
#define AUDIO_LOCATION_SIDE_LEFT                0x00000400  /**< Side Left.*/
#define AUDIO_LOCATION_SIDE_RIGHT               0x00000800  /**< Side Right.*/
#define AUDIO_LOCATION_TOP_FRONT_LEFT           0x00001000  /**< Top Front Left.*/
#define AUDIO_LOCATION_TOP_FRONT_RIGHT          0x00002000  /**< Top Front Right.*/
#define AUDIO_LOCATION_TOP_FRONT_CENTER         0x00004000  /**< Top Front Center.*/
#define AUDIO_LOCATION_TOP_CENTER               0x00008000  /**< Top Center.*/
#define AUDIO_LOCATION_TOP_BACK_LEFT            0x00010000  /**< Top Back Left.*/
#define AUDIO_LOCATION_TOP_BACK_RIGHT           0x00020000  /**< Top Back Right.*/
#define AUDIO_LOCATION_TOP_SIDE_LEFT            0x00040000  /**< Top Side Left.*/
#define AUDIO_LOCATION_TOP_SIDE_RIGHT           0x00080000  /**< Top Side Right.*/
#define AUDIO_LOCATION_TOP_BACK_CENTER          0x00100000  /**< Top Back Center.*/
#define AUDIO_LOCATION_BOTTOM_FRONT_CENTER      0x00200000  /**< Bottom Front Center.*/
#define AUDIO_LOCATION_BOTTOM_FRONT_LEFT        0x00400000  /**< Bottom Front Left.*/
#define AUDIO_LOCATION_BOTTOM_FRONT_RIGHT       0x00800000  /**< Bottom Front Right.*/
#define AUDIO_LOCATION_FRONT_LEFT_WIDE          0x01000000  /**< Front Left Wide.*/
#define AUDIO_LOCATION_FRONT_RIGHT_WIDE         0x02000000  /**< Front Right Wide.*/
#define AUDIO_LOCATION_LEFT_SURROUND            0x04000000  /**< Left Surround.*/
#define AUDIO_LOCATION_RIGHT_SURROUND           0x08000000  /**< Right Surround.*/
typedef uint32_t bt_le_audio_location_t;              /**< The type of audio location.*/

/**
 * @brief Defines for audio content type.
 */
#define AUDIO_CONTENT_TYPE_NOT_AVAILABLE        0x0000  /**< Not available.*/
#define AUDIO_CONTENT_TYPE_UNSPECIFIED          0x0001  /**< Unspecified.*/
#define AUDIO_CONTENT_TYPE_CONVERSATIONAL       0x0002  /**< Conversational.*/
#define AUDIO_CONTENT_TYPE_MEDIA                0x0004  /**< Media.*/
#define AUDIO_CONTENT_TYPE_GAME                 0x0008  /**< Game.*/
#define AUDIO_CONTENT_TYPE_INSTRUCTIONAL        0x0010  /**< Instructional.*/
#define AUDIO_CONTENT_TYPE_VA                   0x0020  /**< Voice assistants.*/
#define AUDIO_CONTENT_TYPE_LIVE                 0x0040  /**< Live.*/
#define AUDIO_CONTENT_TYPE_SOUND_EFFECTS        0x0080  /**< Sound effects.*/
#define AUDIO_CONTENT_TYPE_NOTIFICATION         0x0100  /**< Notification.*/
#define AUDIO_CONTENT_TYPE_RINGTONE             0x0200  /**< Ringtone.*/
#define AUDIO_CONTENT_TYPE_ALERTS               0x0400  /**< Alerts.*/
#define AUDIO_CONTENT_TYPE_EMERGENCY_ALARM      0x0800  /**< Emergency alarm.*/
#define AUDIO_CONTENT_TYPE_ALL                  0x0FFF  /**< All.*/
#define AUDIO_CONTENT_TYPE_ULL_BLE              0x1234  /**< AIROHA proprietary ULL BLE */
#define AUDIO_CONTENT_TYPE_WIRELESS_MIC         0x5678  /**< AIROHA proprietary ULL BLE */
typedef uint16_t bt_le_audio_content_type_t;          /**< The type of audio content.*/

/**
 * @brief Defines for announcement type.
 */
#define ANNOUNCEMENT_TYPE_GENERAL                0x00   /**< Unicast Server is connectable but is not requesting a connection.*/
#define ANNOUNCEMENT_TYPE_TARGETED               0x01   /**< Unicast Server is connectable and is requesting a connection. */
typedef uint8_t bt_le_audio_announcement_type_t;    /**<The type of announcement*/

/**
 * @brief Defines for presentation delay size.
 */
#define PRESENTATION_DELAY_SIZE                 3

/**
 * @brief Defines for sdu interval size.
 */
#define SDU_INTERVAL_SIZE                       3

/**
 * @brief Defines for codec LC3.
 */
#define CODEC_ID_LC3                {6,             0, 0,       0, 0}   /**< Coding_Format, Company_ID, Vendor-specific_codec_ID.*/

/**
 * @brief Defines the length of Codec Specific Capabilties parameters.
 */
#define CODEC_CAPABILITY_LEN_SUPPORTED_SAMPLING_FREQUENCY       0x03    /**< The length of Supported Sampling Frequencies.*/
#define CODEC_CAPABILITY_LEN_SUPPORTED_FRAME_DURATIONS          0x02    /**< The length of Supported Frame Durations.*/
#define CODEC_CAPABILITY_LEN_AUDIO_CHANNEL_COUNTS               0x02    /**< The length of Audio Channel Counts.*/
#define CODEC_CAPABILITY_LEN_SUPPORTED_OCTETS_PER_CODEC_FRAME   0x05    /**< The length of Supported Octets Per Codec Frame.*/
#define CODEC_CAPABILITY_LEN_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU 0X02    /**< The length of Max Supported LC3 Frames Per SDU.*/

/**
 * @brief Defines the type of Codec Specific Capabilties parameters.
 */
enum {
    CODEC_CAPABILITY_TYPE_SUPPORTED_SAMPLING_FREQUENCY = 0x01,  /**< Supported Sampling Frequencies.*/
    CODEC_CAPABILITY_TYPE_SUPPORTED_FRAME_DURATIONS,            /**< Supported Frame Durations.*/
    CODEC_CAPABILITY_TYPE_AUDIO_CHANNEL_COUNTS,                 /**< Audio Channel Counts.*/
    CODEC_CAPABILITY_TYPE_SUPPORTED_OCTETS_PER_CODEC_FRAME,     /**< Supported Octets Per Codec Frame.*/
    CODEC_CAPABILITY_TYPE_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU, /**< Max Supported LC3 Frames Per SDU.*/
};

/**
 * @brief Defines the value of Supported Sampling Frequencies in Codec Specific Capabilties parameters.
 */
#define SUPPORTED_SAMPLING_FREQ_8KHZ            0x0001  /**< 8 kHz. */
#define SUPPORTED_SAMPLING_FREQ_11_025KHZ       0x0002  /**< 11.025 kHz. */
#define SUPPORTED_SAMPLING_FREQ_16KHZ           0x0004  /**< 16 kHz. */
#define SUPPORTED_SAMPLING_FREQ_22_05KHZ        0x0008  /**< 22.05 kHz. */
#define SUPPORTED_SAMPLING_FREQ_24KHZ           0x0010  /**< 24 kHz. */
#define SUPPORTED_SAMPLING_FREQ_32KHZ           0x0020  /**< 32 kHz. */
#define SUPPORTED_SAMPLING_FREQ_44_1KHZ         0x0040  /**< 44.1 kHz. */
#define SUPPORTED_SAMPLING_FREQ_48KHZ           0x0080  /**< 48 kHz. */
#define SUPPORTED_SAMPLING_FREQ_88_2KHZ         0x0100  /**< 88.2 kHz. */
#define SUPPORTED_SAMPLING_FREQ_96KHZ           0x0200  /**< 96 kHz. */
#define SUPPORTED_SAMPLING_FREQ_176_4KHZ        0x0400  /**< 176.4 kHz. */
#define SUPPORTED_SAMPLING_FREQ_192KHZ          0x0800  /**< 192 kHz. */
#define SUPPORTED_SAMPLING_FREQ_384KHZ          0x1000  /**< 384 kHz. */

/**
 * @brief Defines the value of Supported Frame Durations in Codec Specific Capabilties parameters.
 */
#define SUPPORTED_FRAME_DURATIONS_7P5_MS            0x01    /**< 7.5 ms frame duration supported */
#define SUPPORTED_FRAME_DURATIONS_10_MS             0x02    /**< 10 ms frame duration supported */
#define SUPPORTED_FRAME_DURATIONS_7P5_MS_PREFERRD   0x10    /**< 7.5 ms preferred. Valid only when 7.5 ms is supported and 10 ms is supported. Shall not be set to 0b1 if bit 5 is set to 0b1. */
#define SUPPORTED_FRAME_DURATIONS_10_MS_PREFERRD    0x20    /**< 10 ms preferred. Valid only when 7.5 ms is supported and 10 ms is supported. Shall not be set to 0b1 if bit 4 is set to 0b1. */

/**
 * @brief Defines the value of Audio Channel Counts in Codec Specific Capabilties parameters.
 */
#define AUDIO_CHANNEL_COUNTS_1                  0x01    /**< Channel count 1. */
#define AUDIO_CHANNEL_COUNTS_2                  0x02    /**< Channel count 2. */

/**
 * @brief Defines the value of Octets Per Codec Frame in Codec Specific Capabilties parameters.
 * Octet 0-1: Minimum number of octets supported per codec frame.
 * Octet 0-1: Minimum number of octets supported per codec frame.
 */
#define SUPPORTED_OCTETS_PER_CODEC_FRAME_30_40      0x0028001E  /**< 30~40. */
#define SUPPORTED_OCTETS_PER_CODEC_FRAME_45_60      0x003C002D  /**< 45~60. */
#define SUPPORTED_OCTETS_PER_CODEC_FRAME_60_80      0x0050003C  /**< 60~80. */
#define SUPPORTED_OCTETS_PER_CODEC_FRAME_75_155     0x009B004B  /**< 75~155. */

/**
 * @brief Defines the value of Max Supported LC3 Frames Per SDU.
 */
#define MAX_SUPPORTED_LC3_FRAMES_PER_SDU_1      0x01

/**
 * @brief Defines Metadata type.
 */
enum {
    METADATA_TYPE_PREFERRED_AUDIO_CONTEXTS = 0x01, /**< Preferred audio contexts. */
    METADATA_TYPE_STREAMING_AUDIO_CONTEXTS,        /**< Streaming audio contexts. */
    METADATA_TYPE_PROGRAM_INFO,                    /**<Program Info>*/
    METADATA_TYPE_LANGUAGE,                        /**<Language>*/
    METADATA_TYPE_CCID_LIST,                       /**<CCID List>*/
    METADATA_TYPE_PARENTAL_RATING,                 /**<Parental Rating>*/
    METADATA_TYPE_PROGRAM_INFO_URI,                /**<Program Info URI>*/
    METADATA_TYPE_EXTENDED_METADATA = 0xFE,        /**<Extended Metadata>*/
    METADATA_TYPE_VENDOR_SPECIFIC = 0xFF,          /**<Vendor_Specific>*/
};

/**
 * @brief Defines the type of Codec Specific Configuration parameters.
 */
enum {
    CODEC_CONFIGURATION_TYPE_SAMPLING_FREQUENCY = 0x01, /**< Supported Sampling Frequencies.*/
    CODEC_CONFIGURATION_TYPE_FRAME_DURATIONS,           /**< Supported Frame Durations.*/
    CODEC_CONFIGURATION_TYPE_AUDIO_CHANNEL_ALLOCATION,  /**< Audio Channel Counts.*/
    CODEC_CONFIGURATION_TYPE_OCTETS_PER_CODEC_FRAME,    /**< Supported Octets Per Codec Frame.*/
    CODEC_CONFIGURATION_TYPE_CODEC_FRAME_BLOCKS_PER_SDU,        /**< Max Supported LC3 Frames Per SDU.*/
};

/**
 * @brief Defines the length of Codec Specific Configuration parameters.
 */
#define CODEC_CONFIGURATION_LEN_SAMPLING_FREQUENCY          2   /**< Supported Sampling Frequencies.*/
#define CODEC_CONFIGURATION_LEN_FRAME_DURATIONS             2   /**< Supported Frame Durations.*/
#define CODEC_CONFIGURATION_LEN_AUDIO_CHANNEL_ALLOCATION    5   /**< Audio Channel Counts.*/
#define CODEC_CONFIGURATION_LEN_OCTETS_PER_CODEC_FRAME      3   /**< Supported Octets Per Codec Frame.*/
#define CODEC_CONFIGURATION_LEN_CODEC_FRAME_BLOCKS_PER_SDU  2   /**< Codec Frame Blocks Per_SDU.*/

/**
 * @brief Defines the value of Sampling Frequency in Codec Specific Configuration parameters.
 */
enum {
    CODEC_CONFIGURATION_SAMPLING_FREQ_8KHZ =    0x01,           /**< 8 kHz. */
    CODEC_CONFIGURATION_SAMPLING_FREQ_16KHZ =   0x03,           /**< 16 kHz. */
    CODEC_CONFIGURATION_SAMPLING_FREQ_24KHZ =   0x05,           /**< 24 kHz. */
    CODEC_CONFIGURATION_SAMPLING_FREQ_32KHZ =   0x06,           /**< 32 kHz. */
    CODEC_CONFIGURATION_SAMPLING_FREQ_44_1KHZ = 0x07,           /**< 44.1 kHz. */
    CODEC_CONFIGURATION_SAMPLING_FREQ_48KHZ =   0x08,           /**< 48 kHz. */
};

/**
 * @brief Defines the value of Frame_Duration in Codec Specific Configuration parameters.
 */
enum {
    FRAME_DURATIONS_7P5_MS, /**< 7.5 ms.*/
    FRAME_DURATIONS_10_MS,  /**< 10 mx.*/
};

/**
 * @brief Defines the type of Metadata LTV structure parameters.
 */
enum {
    METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS = 0x01,  /**< Preferred Audio Contexts.*/
    METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS,         /**< Streaming Audio Contexts.*/
    METADATA_LTV_TYPE_PROGRAM_INFO,                     /**< Program Info, draft specification and is subject to change.*/
    METADATA_LTV_TYPE_LANGUAGE,                         /**< Language, draft specification and is subject to change.*/
    METADATA_LTV_TYPE_CCID_LIST,                        /**< CCID List, draft specification and is subject to change.*/
    METADATA_LTV_TYPE_PARENTAL_RATING,                  /**< Parental Rating, draft specification and is subject to change.*/
    METADATA_LTV_TYPE_PROGRAM_INFO_URI,                 /**< Program info URI, draft specification and is subject to change.*/
    METADATA_LTV_TYPE_EXTENDED_METADATA = 0xFE,         /**< Extended Metadata.*/
    METADATA_LTV_TYPE_VENDOR_SPECIFIC = 0xFF,           /**< Vendor Specific.*/
};

/**
 * @brief Defines the length of Metadata LTV structure parameters.
 */
#define METADATA_LTV_LEN_PREFERRED_AUDIO_CONTEXTS   3   /**< Preferred Audio Contexts.*/
#define METADATA_LTV_LEN_STREAMING_AUDIO_CONTEXTS   3   /**< Streaming Audio Contexts.*/
#define METADATA_LTV_LEN_LANGUAGE                   4   /**< Language, draft specification and is subject to change.*/
#define METADATA_LTV_LEN_PARENTAL_RATING            2   /**< Parental Rating, draft specification and is subject to change.*/

#endif  /* __BT_LE_AUDIO_DEF_H__ */

