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

#ifndef __BLE_MCS_DEF_H__
#define __BLE_MCS_DEF_H__

#include "bt_type.h"

/**
 * @brief The GMCS service UUID.
 */
#define BT_GATT_UUID16_GENERIC_MEDIA_CONTROL_SERVICE    (0x1849)    /**< Generic media control service. */

/**
 * @brief The MCS service UUID.
 */
#define BT_GATT_UUID16_MEDIA_CONTROL_SERVICE            (0x1848)    /**< Media control service. */

/**
 * @brief The object ID size.
 */
#define BLE_MCS_OBJECT_ID_SIZE                          (6) /**< Object Id size */

/**
* @brief The MCS UUID type.
*/
#define BLE_MCS_UUID_TYPE_MEDIA_CONTROL_SERVICE                 0    /**< Media control service UUID type. */
#define BLE_MCS_UUID_TYPE_CHARC_START                           1    /**< The first characteristic UUID type.*/
#define BLE_MCS_UUID_TYPE_MEDIA_PLAYER_NAME                     BLE_MCS_UUID_TYPE_CHARC_START    /**< Media Player Name UUID type. */
#define BLE_MCS_UUID_TYPE_MEDIA_PLAYER_ICON_OBJECT_ID           2    /**< Media Player Icon Object ID UUID type. */
#define BLE_MCS_UUID_TYPE_MEDIA_PLAYER_ICON_URL                 3    /**< Media Player Icon URL UUID type. */
#define BLE_MCS_UUID_TYPE_TRACK_CHANGED                         4    /**< Track Changed UUID type. */
#define BLE_MCS_UUID_TYPE_TRACK_TITLE                           5    /**< Track Title UUID type. */
#define BLE_MCS_UUID_TYPE_TRACK_DURATION                        6    /**< Track duration UUID type. */
#define BLE_MCS_UUID_TYPE_TRACK_POSITION                        7    /**< Track Position UUID type. */
#define BLE_MCS_UUID_TYPE_PLAYBACK_SPEED                        8    /**< Playback Speed UUID type. */
#define BLE_MCS_UUID_TYPE_SEEKING_SPEED                         9    /**< Seeking Speed UUID type. */
#define BLE_MCS_UUID_TYPE_CURRENT_TRACK_SEGMENTS_OBJECT_ID      10   /**< Track Segments Object ID UUID type. */
#define BLE_MCS_UUID_TYPE_CURRENT_TRACK_OBJECT_ID               11   /**< Current Track Object ID UUID type. */
#define BLE_MCS_UUID_TYPE_NEXT_TRACK_OBJECT_ID                  12   /**< Next Track Object ID UUID type. */
#define BLE_MCS_UUID_TYPE_PARENT_GROUP_OBJECT_ID                13   /**< Parent Group Object ID UUID type. */
#define BLE_MCS_UUID_TYPE_CURRENT_GROUP_OBJECT_ID               14   /**< Current Group Object ID UUID type. */
#define BLE_MCS_UUID_TYPE_PLAYING_ORDER                         15   /**< Playing Order UUID type. */
#define BLE_MCS_UUID_TYPE_PLAYING_ORDERS_SUPPORTED              16   /**< Playing Orders Supprted UUID type. */
#define BLE_MCS_UUID_TYPE_MEDIA_STATE                           17   /**< Media State UUID type. */
#define BLE_MCS_UUID_TYPE_MEDIA_CONTROL_POINT                   18   /**< Media Control Point UUID type. */
#define BLE_MCS_UUID_TYPE_MEDIA_CONTROL_OPCODES_SUPPORTED       19   /**< Media Control Opcodes Supported UUID type. */
#define BLE_MCS_UUID_TYPE_SEARCH_CONTROL_POINT                  20   /**< Search Control Point UUID type. */
#define BLE_MCS_UUID_TYPE_SEARCH_RESULTS_OBJECT_ID              21   /**< Search Results Object ID UUID type. */
#define BLE_MCS_UUID_TYPE_CONTENT_CONTROL_ID                    22   /**< Content Control ID UUID type. */
#define BLE_MCS_UUID_TYPE_MAX_NUM                               23   /**< MCS Max number UUID type. */
#define BLE_MCS_UUID_TYPE_GENERIC_MEDIA_CONTROL_SERVICE         0xE0 /**< Generic media control service UUID type. */
#define BLE_MCS_UUID_TYPE_OBJECT_TRANSFER_SERVICE               0xF0 /**< Object transfer service UUID type. */
#define BLE_MCS_UUID_TYPE_INVALID                               0xFF /**< Invalid UUID type.  */
typedef uint8_t ble_mcs_uuid_t;                                      /**< The MCS/GMCS UUID type. */


/**
 * @brief The max number of characteristics.
 */
#define BLE_MCS_MAX_CHARC_NUMBER                 (BLE_MCS_UUID_TYPE_MAX_NUM-1)   /**< The max number of MCS/GMCS characteristics.*/

/**
* @brief The media state of the player.
*/
#define BLE_MCS_MEDIA_STATE_INACTIVE             0x00    /**< The current track is invalid, and no track has been selected.*/
#define BLE_MCS_MEDIA_STATE_STOPED               BLE_MCS_MEDIA_STATE_INACTIVE /**< The state of the media player is stopped. */
#define BLE_MCS_MEDIA_STATE_PLAYING              0x01    /**< The media player is playing the current track. */
#define BLE_MCS_MEDIA_STATE_PAUSED               0x02    /**< The current track is paused. The media player has a current track, but it is not being played. */
#define BLE_MCS_MEDIA_STATE_SEEKING              0x03    /**< The current track is fast forwarding or fast rewinding. */
typedef uint8_t ble_mcs_media_state_t;

/**
* @brief The playing order of the player.
*/
#define BLE_MCS_PLAYING_ORDER_INVALID            0       /**< Invalid playing order. */
#define BLE_MCS_PLAYING_ORDER_SINGLE_ONCE        0x01    /**< A single track is played once; there is no next track. */
#define BLE_MCS_PLAYING_ORDER_SINGLE_REPEAT      0x02    /**< A single track is played repeatedly; the next track is the current track. */
#define BLE_MCS_PLAYING_ORDER_IN_ORDER_ONCE      0x03    /**< The tracks within a group are played once in track order. */
#define BLE_MCS_PLAYING_ORDER_IN_ORDER_REPEAT    0x04    /**< The tracks within a group are played in track order repeatedly.*/
#define BLE_MCS_PLAYING_ORDER_OLDEST_ONCE        0x05    /**< The tracks within a group are played once only from the oldest first.*/
#define BLE_MCS_PLAYING_ORDER_OLDEST_REPEAT      0x06    /**< The tracks within a group are played from the oldest first repeatedly.*/
#define BLE_MCS_PLAYING_ORDER_NEWEST_ONCE        0x07    /**< The tracks within a group are played once only from the newest first.*/
#define BLE_MCS_PLAYING_ORDER_NEWEST_REPEAT      0x08    /**< The tracks within a group are played from the newest first repeatedly. */
#define BLE_MCS_PLAYING_ORDER_SHUFFLE_ONCE       0x09    /**< The tracks within a group are played in random order once.*/
#define BLE_MCS_PLAYING_ORDER_SHUFFLE_REPEAT     0x0A    /**< The tracks within a group are played in random order repeatedly.*/
typedef uint8_t ble_mcs_playing_order_t;                 /**< The playing order type. */

/**
* @brief The playing order supported of the player.
*/
#define BLE_MCS_PLAYING_ORDER_SUPPORTED_INVALID            0         /**< Invalid playing order supported. */
#define BLE_MCS_PLAYING_ORDER_SUPPORTED_SINGLE_ONCE        0x0001    /**< A single track is played once; there is no next track. */
#define BLE_MCS_PLAYING_ORDER_SUPPORTED_SINGLE_REPEAT      0x0002    /**< A single track is played repeatedly; the next track is the current track.*/
#define BLE_MCS_PLAYING_ORDER_SUPPORTED_IN_ORDER_ONCE      0x0004    /**< The tracks within a group are played once in track order.*/
#define BLE_MCS_PLAYING_ORDER_SUPPORTED_IN_ORDER_REPEAT    0x0008    /**< The tracks within a group are played in track order repeatedly.*/
#define BLE_MCS_PLAYING_ORDER_SUPPORTED_OLDEST_ONCE        0x0010    /**< The tracks within a group are played once only from the oldest first. */
#define BLE_MCS_PLAYING_ORDER_SUPPORTED_OLDEST_REPEAT      0x0020    /**< The tracks within a group are played from the oldest first repeatedly. */
#define BLE_MCS_PLAYING_ORDER_SUPPORTED_NEWEST_ONCE        0x0040    /**< The tracks within a group are played once only from the newest first.*/
#define BLE_MCS_PLAYING_ORDER_SUPPORTED_NEWEST_REPEAT      0x0080    /**< The tracks within a group are played from the newest first repeatedly. */
#define BLE_MCS_PLAYING_ORDER_SUPPORTED_SHUFFLE_ONCE       0x0100    /**< The tracks within a group are played in random order once.*/
#define BLE_MCS_PLAYING_ORDER_SUPPORTED_SHUFFLE_REPEAT     0x0200    /**< The tracks within a group are played in random order repeatedly.*/
typedef uint16_t ble_mcs_playing_order_supported_t;                  /**< The mediay playing order supported type. */

/**
* @brief The search type.
*/
#define BLE_MCS_SEARCH_TYPE_TRACK_NAME           0x01    /**< Track Name.*/
#define BLE_MCS_SEARCH_TYPE_ARTIST_NAME          0x02    /**< Artist Name. */
#define BLE_MCS_SEARCH_TYPE_ALBUM_NAME           0x03    /**< Album Name */
#define BLE_MCS_SEARCH_TYPE_GROUP_NAME           0x04    /**< Group Name. */
#define BLE_MCS_SEARCH_TYPE_EARLIEST_YEAR        0x05    /**< Earliest Year*/
#define BLE_MCS_SEARCH_TYPE_LATEST_YEAR          0x06    /**< Latest Year.*/
#define BLE_MCS_SEARCH_TYPE_GENRE                0x07    /**< Genre. */
#define BLE_MCS_SEARCH_TYPE_ONLY_TRACKS          0x08    /**< Only Tracks. */
#define BLE_MCS_SEARCH_TYPE_ONLY_GROUPS          0x09    /**< Only Groups. */
typedef uint8_t ble_mcs_search_t;                        /**< The meida search type */

/**
* @brief The media control opcodes supported.
*/
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_INVALID              0           /**< Invalid media control opcodes supported. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_PLAY                 0x000001    /**< Play. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_PAUSE                0x000002    /**< Pause. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_FAST_FORWARD         0x000004    /**< Fast Forward.*/
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_FAST_REWIND          0x000008    /**< Fast Rewind*/
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_STOP                 0x000010    /**< Stop. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_MOVE_RELATIVE        0x000020    /**< Move Relative*/
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_PREVIOUS_SEGMENT     0x000040    /**< Previous Segment. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_NEXT_SEGMENT         0x000080    /**< Next Segment.  */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_FIRST_SEGMENT        0x000100    /**< First Segment. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_LAST_SEGMENT         0x000200    /**< Last Segment. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_GOTO_SEGMENT         0x000400    /**< Goto Segment. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_PREVIOUS_TRACK       0x000800    /**< Previous Track.*/
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_NEXT_TRACK           0x001000    /**< Next Track.*/
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_FIRST_TRACK          0x002000    /**< First Track. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_LAST_TRACK           0x004000    /**< Last Track.*/
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_GOTO_TRACK           0x008000    /**< Goto Track. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_PREVIOUS_GROUP       0x010000    /**< Previous Group. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_NEXT_GROUP           0x020000    /**< Next Group. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_FIRST_GROUP          0x040000    /**< First Group. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_LAST_GROUP           0x080000    /**< Last Group. */
#define BLE_MCS_MEDIA_CONTROL_OPCODES_SUPPORTED_GOTO_GROUP           0x100000    /**< Goto Group.*/
typedef uint32_t ble_mcs_media_control_opcodes_supported_t;                      /**< Then meida control opcodes supported type. */

/**
* @brief The media control point opcode.
*/
#define BLE_MCS_MEDIA_CONTROL_POINT_PLAY                 0x01    /**< Start playing the current track.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_PAUSE                0x02    /**< Pause playing the current track.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_FAST_REWIND          0x03    /**< Fast rewind the current track.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_FAST_FORWARD         0x04    /**< Fast forward the current track.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_STOP                 0x05    /**< Stop current activity and return to the paused state and set the current track position to the start of the current track.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_MOVE_RELATIVE        0x10    /**< Set a new current track position relative to the current track position.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_PREVIOUS_SEGMENT     0x20    /**< Set the current track position to the starting position of the previous segment of the current track.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_NEXT_SEGMENT         0x21    /**< Set the current track position to the starting position of the next segment of the current track.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_FIRST_SEGMENT        0x22    /**< Set the current track position to the starting position of the first segment of the current track.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_LAST_SEGMENT         0x23    /**< Set the current track position to the starting position of the last segment of the current track.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_GOTO_SEGMENT         0x24    /**< Set the current track position to the starting position of the nth segment of the current track. */
#define BLE_MCS_MEDIA_CONTROL_POINT_PREVIOUS_TRACK       0x30    /**< Set the current track to the previous track based on the playing order.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_NEXT_TRACK           0x31    /**< Set the current track to the next track based on the playing order.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_FIRST_TRACK          0x32    /**< Set the current track to the first track based on the playing order.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_LAST_TRACK           0x33    /**< Set the current track to the last track based on the playing order.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_GOTO_TRACK           0x34    /**< Set the current track to the nth track based on the playing order. */
#define BLE_MCS_MEDIA_CONTROL_POINT_PREVIOUS_GROUP       0x40    /**< Set the current group to the previous group in the sequence of groups.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_NEXT_GROUP           0x41    /**< Set the current group to the next group in the sequence of groups.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_FIRST_GROUP          0x42    /**< Set the current group to the first group in the sequence of groups.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_LAST_GROUP           0x43    /**< Set the current group to the last group in the sequence of groups.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_GOTO_GROUP           0x44    /**< Set the current group to the nth group in the sequence of groups. */
#define BLE_MCS_MEDIA_CONTROL_POINT_INVALID              0xFF    /**< Invalid media control point. */
typedef uint8_t ble_mcs_media_control_point_t;                   /**< The meida control porint type. */

/**
* @brief The media control point result.
*/
#define BLE_MCS_MEDIA_CONTROL_POINT_RESULT_SUCCESS                       0x01    /**< Action requested by the opcode write was completed successfully.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_RESULT_OPCODE_NOT_SUPPORTED          0x02    /**< An invalid or unsupported opcode was used for the Media Control Point write.*/
#if 1 /* __MTK_MCP_COMMON__: In MCP spec., 0x03 is INACTIVE state, and 0x04 is UNSUCCESSFUL state. */
#define BLE_MCS_MEDIA_CONTROL_POINT_RESULT_MEDIA_PLAYER_INACTIVE         0x03    /**< The Media Player State characteristic value is Inactive when the opcode is received or the result of the requested action of the opcode results in the Media Player State characteristic being set to Inactive.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_RESULT_OPCODE_ACTION_UNSUCCESSFUL    0x04    /**< The requested action of any Media Control Point write cannot be completed successfully because of a condition within the player. */
#else
#define BLE_MCS_MEDIA_CONTROL_POINT_RESULT_OPCODE_ACTION_UNSUCCESSFUL    0x03    /**< The Media Player State characteristic value is Inactive when the opcode is received or the result of the requested action of the opcode results in the Media Player State characteristic being set to Inactive.*/
#define BLE_MCS_MEDIA_CONTROL_POINT_RESULT_MEDIA_PLAYER_INACTIVE         0x04    /**< The requested action of any Media Control Point write cannot be completed successfully because of a condition within the player. */
#endif /* __MTK_MCP_COMMON__ */
typedef uint8_t ble_mcs_media_control_point_result_t;                            /**< The media control point result type. */

/**
 * @brief Defines for MCS attribute handle.
 */
typedef struct {
    ble_mcs_uuid_t uuid_type;   /**< The UUID type. */
    uint16_t att_handle;        /**< The attirbute handle. */
} ble_mcs_attribute_handle_t;

#endif  /* __BLE_MCS_DEF_H__ */

