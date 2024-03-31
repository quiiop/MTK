/* Copyright Statement:
 *
 * (C) 2005-2021  MediaTek Inc. All rights reserved.
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

#ifndef __BT_SINK_SRV_MUSIC_H__
#define __BT_SINK_SRV_MUSIC_H__

#include "stdint.h"
#include "bt_type.h"
#include "bt_connection_manager_internal.h"
#include "bt_sink_srv_ami.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "hal_gpt.h"
#include "bt_sink_srv.h"
#include "bt_avm.h"

#ifdef MTK_BT_TIMER_EXTERNAL_ENABLE
#include "bt_timer_external.h"
#endif /* #ifdef MTK_BT_TIMER_EXTERNAL_ENABLE */
#include "audio_src_srv.h"

#include "bt_system.h"
#include "bt_avrcp.h"

//#define BT_SINK_SRV_CONTROL_MUSIC_BY_AVRCP_STATUS

//#define BT_SINK_SRV_SET_AUDIO_CHANNEL_BY_SINK_MUSIC

#define __BT_SINK_SRV_A2DP_V13_SUPPORT__

#define BT_SINK_SRV_AVRCP_CONNECTION_TIMER_DUR             (1000)
#define BT_SINK_SRV_AVRCP_DISCONNECT_TIMER_DUR             (1000)
#define BT_SINK_SRV_A2DP_START_STREAMING_TIMER_DUR         (1000)
#define BT_SINK_SRV_REINITIAL_SYNC_TIMER_DUR               (2000)
#define BT_SINK_SRV_AVRCP_PLAY_PAUSE_ACTION_TIMER_DUR       (5000)
#define BT_SINK_SRV_AVRCP_SET_VOLUME_TIMER_DUR              (80)
#define BT_SINK_SRV_AVRCP_PUSH_RELEASE_TIMER_DUR            (45)
#define BT_SINK_SRV_AVRCP_CONNECTION_AS_TG_TIMER_DUR       (1)

#define BT_SINK_SRV_MAX_RATIO (0xf)
#define BT_SINK_SRV_MIN_RATIO (0x1)
#define BT_SINK_SRV_MAX_SAMPLE_COUNT_PER_PKT (0xffff)
#define BT_SINK_SRV_SAMPLE_COUNT_PER_PKT_AAC (1024)
#define BT_SINK_SRV_IPHONE_TYPE_IN_RATIO (0X0)
#define BT_NCLK_MASK          (0x0FFFFFFF)
#define BT_SINK_SRV_INVALID_SEQUENCE_NUMBER (0xFFFFFFFF)
#define BT_SINK_SRV_PCB_STATE_COMPLETE (1)
#define BT_SINK_SRV_PCB_STATE_INCOMPLETE    (2)
#define BT_SINK_SRV_SPECIAL_DEV_MAGIC_NUMBER  (0xffff)
#define BT_SINK_SRV_MUSIC_NW_A45_DEV_FLAG       (0x01)
#define BT_SINK_SRV_MUSIC_NOTIFY_N9_NO_SLEEP    (0x02)


#define BT_SINK_SRV_A2DP_STATUS_SUSPEND     0x00
#define BT_SINK_SRV_A2DP_STATUS_STREAMING   0x01

#define BT_SINK_SRV_MUSIC_AVRCP_INVALID_STATUS (0xff)
#define BT_SINK_SRV_MUSIC_AVRCP_NO_OPERATION   (0xff)
#define BT_SINK_SRV_MUSIC_AVRCP_NO_STATE       (0xff)
#define BT_SINK_SRV_INVALID_LAST_PLAY_PAUSE_ACTION (0x0)

#define BT_SINK_SRV_A2DP_VOL_DEF_LEV            (AUD_VOL_OUT_LEVEL6)
#define BT_SINK_SRV_A2DP_NO_ABSOLUTE_VOL_DEF_LEV            (AUD_VOL_OUT_LEVEL13)

#define BT_SINK_SRV_CNTX_FLAG_RECONNECT_AGENT_FLAG          (1<<0)
#define BT_SINK_SRV_CNTX_FLAG_POWER_OFF                     (1<<1)
#define BT_SINK_SRV_CNTX_FLAG_A2DP_INTER_ULL_BY_AVRCP       (1<<2)
#define BT_SINK_SRV_CNTX_FLAG_SWITCH_AUDIO_OFF              (1<<3)
#define BT_SINK_SRV_CNTX_FLAG_MUST_PLAY_RING_TONE_FLAG      BT_SINK_SRV_AVRCP_MUST_PLAY_RING_TONE_FLAG //(1 << 6)
typedef uint8_t bt_sink_srv_ctx_flag_t;

/**< Deveice flag */
/* Stable flag */
#define BT_SINK_SRV_MUSIC_FLAG_ADD_WAITING_LIST             (1 << 0)
#define BT_SINK_SRV_MUSIC_NORMAL_STOP_FLAG                  (1 << 1)
#define BT_SINK_SRV_MUSIC_FLAG_A2DP_CON_IND                 (1 << 2)
#define BT_SINK_SRV_MUSIC_FLAG_DEV_TO_CON_AVRCP             (1 << 3)
#define BT_SINK_SRV_MUSIC_FLAG_A2DP_INTERRUPT               (1 << 4)
#define BT_SINK_SRV_MUSIC_FLAG_WAITING_START                (1 << 5)
#define BT_SINK_SRV_MUSIC_FLAG_REINITIAL_SYNC               (1 << 6)
#define BT_SINK_SRV_MUSIC_INITIAL_A2DP_BY_DEVICE            (1 << 7)
#define BT_SINK_SRV_MUSIC_WAIT_SET_VOLUME                   (1 << 8)
#define BT_SINK_SRV_MUSIC_NEED_TO_RESPONSE_A2DP_START       (1 << 9)
#define BT_SINK_SRV_MUSIC_STOP_MUSIC_PENDING                (1 << 10)

/*avrcp flag*/
#define BT_SINK_SRV_AVRCP_GET_ELEMENT_ATTRIBUTE_FRAGMENT_SUPPORT    (1 << 0)
#define BT_SINK_SRV_AVRCP_GET_FOLDER_ITEM                           (1 << 1)
#define BT_SINK_SRV_INIT_AVRCP_BY_REMOTE_DEVICE                     (1 << 2)
#define BT_SINK_SRV_AVRCP_WAITING_FORCE_PAUSE_FLAG                  (1 << 3)
#define BT_SINK_SRV_AVRCP_MUST_PLAY_RING_TONE_FLAG                  (1 << 4)

/* Wait flag */
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_START                   (BT_SINK_SRV_MUSIC_STOP_MUSIC_PENDING << 1)
// 1<<16
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_AMI_OPEN_CODEC          ((BT_SINK_SRV_MUSIC_FLAG_WAIT_START) << 0)
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_DRV_PLAY                ((BT_SINK_SRV_MUSIC_FLAG_WAIT_START) << 1)
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_DRV_STOP                ((BT_SINK_SRV_MUSIC_FLAG_WAIT_START) << 2)
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_SINK_START              ((BT_SINK_SRV_MUSIC_FLAG_WAIT_START) << 3)
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_LIST_SINK_PLAY          ((BT_SINK_SRV_MUSIC_FLAG_WAIT_START) << 4)    /**< Wait to play in waiting list*/
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_MP3_CHANGE_TRACK        ((BT_SINK_SRV_MUSIC_FLAG_WAIT_START) << 6)    /**< indicate change track */
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_PSEDEV_PLAYING          ((BT_SINK_SRV_MUSIC_FLAG_WAIT_START) << 8)
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_BT_CLK_OFFSET           ((BT_SINK_SRV_MUSIC_FLAG_WAIT_START) << 9)
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_START_GPT_TIMER         ((BT_SINK_SRV_MUSIC_FLAG_WAIT_START) << 10)
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_GPT_TIMEOUT             ((BT_SINK_SRV_MUSIC_FLAG_WAIT_START) << 12)
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_N_PKT_DONE              ((BT_SINK_SRV_MUSIC_FLAG_WAIT_START) << 14)
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_AVRCP_CONN_RESULT        ((BT_SINK_SRV_MUSIC_FLAG_WAIT_START) << 15)
#define BT_SINK_SRV_MUSIC_FLAG_WAIT_ALL                     ((BT_SINK_SRV_MUSIC_FLAG_WAIT_AMI_OPEN_CODEC) | \
                                                              (BT_SINK_SRV_MUSIC_FLAG_WAIT_DRV_PLAY)     | \
                                                              (BT_SINK_SRV_MUSIC_FLAG_WAIT_DRV_STOP)     | \
                                                              (BT_SINK_SRV_MUSIC_FLAG_WAIT_SINK_START)   | \
                                                              (BT_SINK_SRV_MUSIC_FLAG_WAIT_SINK_SUSPEND))

/**< Operation flag */
#define BT_SINK_SRV_MUSIC_OP_CODEC_OPEN                     (1 << 0)    /**< Codec open */
#define BT_SINK_SRV_MUSIC_OP_DRV_PLAY                       (1 << 2)    /**< DRV play done */
#define BT_SINK_SRV_MUSIC_OP_PLAY_IND                       (1 << 3)
/* Next action operation flag */
#define BT_SINK_SRV_MUSIC_OP_AUTO_SYNC                      (1 << 16)   /**< Pending auto sync */

#define BT_SINK_SRV_MUSIC_AVRCP_PLAY_TRIGGER                (1 << 18)   /**< AVRCP trigger, no need RSP */

#define BT_SINK_SRV_MUSIC_A2DP_HF_INTERRUPT                 (1 << 25)   /**< HF interrupt A2DP, but no avrcp pause status change */
#define BT_SINK_SRV_MUSIC_REINIT_ON_PARTNER_LATER_JOIN_FLAG (1 << 26)   /*< Use this flag to rember need to do reinit sync when partner later join*/
#define BT_SINK_SRV_MUSIC_START_PLAY_ON_NEW_AGENT_FLAG      (1 << 27)   /*< This flag should be set if a2dp start come during RHO, so need to resume music on new agent*/
#define BT_SINK_SRV_MUSIC_NO_STREAMING_STATE_UPDATE_FLAG    (1 << 28)
#define BT_SINK_SRV_MUSIC_INIT_DISCONNNECT_AVRCP_TIMER_FLAG (1 << 29)   /*< This flag should be set if avrcp connection action is send before a2dp disconnect ind*/
#define BT_SINK_SRV_MUSIC_GAME_FLAG                         (1 << 30)   /*< If this flag is set, it means it`s on playing gaming*/
#define BT_SINK_SRV_MUSIC_WAITING_REINITIAL_SYNC            (1 << 31)

#define BT_SINK_SRV_MUSIC_PREPARE_BUF_THRESHOLD             (120 * 1000) /**< 120ms cache */


#define BT_SINK_SRV_MUSIC_MAX_LOG_COUNT              (10)
#define BT_SINK_SRV_MUSIC_A2DP_CONN_BIT              (1 << 0)
#define BT_SINK_SRV_MUSIC_AVRCP_CONN_BIT             (1 << 1)
#define BT_SINK_SRV_MUSIC_AVRCP_BROWSING_CONN_BIT    (1 << 2)

#define BT_SINK_SRV_MUSIC_INVALID_HD                 (0x00)

#define BT_SINK_SRV_INVALID_AID                 (-1)

#define BT_SINK_SRV_MEDIA_PKT_HEADER_LEN        (12)
#define BT_SINK_SRV_MEDIA_SBC_SYNC_WORD         (0x3453)
#define BT_SINK_SRV_MEDIA_SBC_SYNC_WORD_LEN     (4)

#define BT_SINK_SRV_AAC_ADTS_LENGTH             (7)


#define BT_SINK_SRV_MUSIC_ERR_SUCCESS_7TH            (7)
#define BT_SINK_SRV_MUSIC_ERR_SUCCESS_6TH            (6)
#define BT_SINK_SRV_MUSIC_ERR_SUCCESS_5TH            (5)
#define BT_SINK_SRV_MUSIC_ERR_SUCCESS_4TH            (4)
#define BT_SINK_SRV_MUSIC_ERR_SUCCESS_3RD            (3)
#define BT_SINK_SRV_MUSIC_ERR_SUCCESS_2ND            (2)
#define BT_SINK_SRV_MUSIC_ERR_SUCCESS_1ST            (1)
#define BT_SINK_SRV_MUSIC_ERR_SUCCESS_OK             (0)
#define BT_SINK_SRV_MUSIC_ERR_FAIL_1ST               (-1)
#define BT_SINK_SRV_MUSIC_ERR_FAIL_2ND               (-2)
#define BT_SINK_SRV_MUSIC_ERR_FAIL_3RD               (-3)
#define BT_SINK_SRV_MUSIC_ERR_FAIL_4TH               (-4)
#define BT_SINK_SRV_MUSIC_ERR_FAIL_5TH               (-5)
#define BT_SINK_SRV_MUSIC_ERR_FAIL_6TH               (-6)
#define BT_SINK_SRV_MUSIC_ERR_FAIL_7TH               (-7)

#ifndef MAXIMUM
#define     MAXIMUM(A, B)                  (((A)>(B))?(A):(B))
#define     MINIMUM(A, B)                  (((A)<(B))?(A):(B))
#endif /* #ifndef MAXIMUM */

#define BT_SINK_SRV_SET_FLAG(FLAG, MASK) do { \
    (FLAG) |= (MASK); \
} while(0);

#define BT_SINK_SRV_REMOVE_FLAG(FLAG, MASK) do { \
    (FLAG) &= ~(MASK); \
} while(0);


#define BT_SINK_SRV_MUISC_DEV_COUNT     (4)     /**< Max device count */

#define BT_SINK_SRV_A2DP_DATA_RCE_LOOP_COUNT                       (300)

#define BT_SINK_SRV_A2DP_DATA_ERROR_LOOP_COUNT                     (100)

#define BT_SINK_SRV_MUSIC_DUR_MIN                                  (70 * 1000)
#define BT_SINK_SRV_MUSIC_DUR_MAX                                  (500 * 1000)

#define BT_SINK_SRV_MUSIC_NEXT_DUR_MIN                             (10 * 1000)
#define BT_SINK_SRV_MUSIC_NEXT_DUR_MAX                             (1000 * 1000)

#define BT_SINK_SRV_MAX_OPTIMIZE_MEDIA_PACKET_COUNT                 (10)
#define BT_SINK_SRV_A2DP_DEFAULT_SINK_LATENCY                       (10000000) // Default 100 ms

#define BT_SINK_SRV_A2DP_BT_VOL_LEV                                 (100)
#define BT_SINK_SRV_A2DP_LOCAL_VOL_LEV                              (16)

#ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__
//#define BT_SINK_SRV_A2DP_DELAY          (((BT_SINK_SRV_A2DP_AVM_TIMER_DUR_BY_TICK*625)>>1 + (BT_SINK_SRV_A2DP_N_PACKET_NOTIFY * 15 * 1000)) / 100)
#define BT_SINK_SRV_A2DP_DELAY          (1000) /**100 ms*/
#endif /* #ifdef __BT_SINK_SRV_A2DP_V13_SUPPORT__ */

typedef enum {
    BT_SINK_SRV_MUSIC_DEVICE_A2DP_HD,                       /* 0 */
    BT_SINK_SRV_MUSIC_DEVICE_AVRCP_HD,                      /* 1 */
    BT_SINK_SRV_MUSIC_DEVICE_ADDR_A2DP,                     /* 2 */
    BT_SINK_SRV_MUSIC_DEVICE_ADDR_AVRCP,                    /* 3 */
    BT_SINK_SRV_MUSIC_DEVICE_UNUSED,                        /* 4 */
    BT_SINK_SRV_MUSIC_DEVICE_USED,                          /* 5 */
    BT_SINK_SRV_MUSIC_DEVICE_LATEST,                        /* 6 */
    BT_SINK_SRV_MUSIC_DEVICE_INTERRUPT,                     /* 7 */
    BT_SINK_SRV_MUSIC_DEVICE_FOCUS,                         /* 8 */
    BT_SINK_SRV_MUSIC_DEVICE_PSE_HD,                        /**< 9 Pseudo device handle */
    BT_SINK_SRV_MUSIC_DEVICE_SP,                            /**< 10 */
    BT_SINK_SRV_MUSIC_DEVICE_LOCAL,                         /**< 11 */
    BT_SINK_SRV_MUSIC_DEVICE_AVRCP_BROWSE_HD                /**< 12 */
} bt_sink_srv_music_device_type_t;

typedef audio_src_srv_state_t bt_sink_srv_music_state_t;

typedef enum {
    BT_SINK_SRV_MUSIC_TRANSIENT_STATE_NONE = 0,
    BT_SINK_SRV_MUSIC_TRANSIENT_STATE_WAIT_CONN,
    BT_SINK_SRV_MUSIC_TRANSIENT_STATE_WAIT_DISCONN,
    BT_SINK_SRV_MUSIC_TRANSIENT_STATE_PREPARE_CODEC,
    BT_SINK_SRV_MUSIC_TRANSIENT_STATE_PREPARE_BUFFER,
    BT_SINK_SRV_MUSIC_TRANSIENT_STATE_WAIT_INIT_SYNC,       /**< waiting init sync */
    BT_SINK_SRV_MUSIC_TRANSIENT_STATE_CLEAR_CODEC,
    /**< waiting stop cnf */
} bt_sink_srv_music_transient_state_t;


#define BT_SINK_SRV_MUSIC_EVT_BASE              (0x800000)

#define BT_SINK_SRV_MUSIC_DATA_VOLUME           (0x00)
#define BT_SINK_SRV_MUSIC_DATA_LOCAL_ASI_FLAG   (0x02)

typedef uint8_t bt_sink_srv_music_data_type_t;

typedef uint32_t bt_sink_srv_music_event_t;

#define BT_SINK_SRV_MUSIC_EVT_START             (BT_SINK_SRV_MUSIC_EVT_BASE)
#define BT_SINK_SRV_MUSIC_EVT_UNAVAILABLE       (BT_SINK_SRV_MUSIC_EVT_START + 1)
#define BT_SINK_SRV_MUSIC_EVT_READY             (BT_SINK_SRV_MUSIC_EVT_START + 2)
#define BT_SINK_SRV_MUSIC_EVT_PLAYING           (BT_SINK_SRV_MUSIC_EVT_START + 3)
#define BT_SINK_SRV_MUSIC_EVT_REJECT            (BT_SINK_SRV_MUSIC_EVT_START + 4)
#define BT_SINK_SRV_MUSIC_EVT_CODEC_OPEN        (BT_SINK_SRV_MUSIC_EVT_START + 5)
#define BT_SINK_SRV_MUSIC_EVT_PREPARE_FAIL      (BT_SINK_SRV_MUSIC_EVT_START + 6)
#define BT_SINK_SRV_MUSIC_EVT_CODEC_CLEAR       (BT_SINK_SRV_MUSIC_EVT_START + 7)
#define BT_SINK_SRV_MUSIC_EVT_RECOVER           (BT_SINK_SRV_MUSIC_EVT_START + 8)
#define BT_SINK_SRV_MUSIC_EVT_RESUME            (BT_SINK_SRV_MUSIC_EVT_START + 9)
#define BT_SINK_SRV_MUSIC_EVT_START_IND         (BT_SINK_SRV_MUSIC_EVT_START + 10)
#define BT_SINK_SRV_MUSIC_EVT_START_CNF         (BT_SINK_SRV_MUSIC_EVT_START + 11)
#define BT_SINK_SRV_MUSIC_EVT_SUSPEND_IND       (BT_SINK_SRV_MUSIC_EVT_START + 12)
#define BT_SINK_SRV_MUSIC_EVT_SUSPEND_CNF       (BT_SINK_SRV_MUSIC_EVT_START + 13)
#define BT_SINK_SRV_MUSIC_EVT_PREPARE_CODEC     (BT_SINK_SRV_MUSIC_EVT_START + 15)
#define BT_SINK_SRV_MUSIC_EVT_PREPARE_CLEAR     (BT_SINK_SRV_MUSIC_EVT_START + 16)

#define BT_SINK_SRV_MUSIC_EVT_AVRCP_PLAYING     (BT_SINK_SRV_MUSIC_EVT_START + 20)
#define BT_SINK_SRV_MUSIC_EVT_AVRCP_PAUSED      (BT_SINK_SRV_MUSIC_EVT_START + 21)

#define BT_SINK_SRV_MUSIC_PLAYBACK_STATE_NONE                    (0x00)
#define BT_SINK_SRV_MUSIC_PLAYBACK_STATE_STOPPED                 (0x01)
#define BT_SINK_SRV_MUSIC_PLAYBACK_STATE_PLAYING                 (0x02)
typedef uint8_t bt_sink_srv_music_playback_state_t;

typedef void(* bt_sink_srv_music_callback_t)(uint32_t evt_id, void *param, void *user_data);

typedef struct {
    uint32_t asi;
    bt_clock_t clock;
    uint32_t ratio;
    uint32_t samples;
} bt_sink_srv_music_data_info_t;

typedef struct {
    bt_sink_srv_music_state_t state;
    bt_sink_srv_music_state_t target_state;
    bt_bd_addr_t dev_addr;
    uint16_t conn_bit;
    uint32_t flag;                                          /**< Device flag */
    uint32_t op;                                            /**< Operation flag */
    uint32_t avrcp_flag;                                    /**< avrcp related flag*/
    audio_src_srv_handle_t *handle;                         /**< Pseudo device handle */

    bt_sink_srv_am_media_handle_t med_handle;
    bt_a2dp_codec_capability_t codec;
    /* A2DP members */
    uint32_t a2dp_hd;
    bt_a2dp_role_t role;
    /* AVRCP members */
    uint32_t avrcp_hd;
    bt_avrcp_operation_id_t avrcp_cmd_action_id;
    uint32_t avrcp_browse_hd;
    uint8_t avrcp_status;
    uint8_t a2dp_status;
    bool volume_change_status;   /* record SP whether register volume change notification */
    bt_avrcp_operation_id_t last_play_pause_action;
    bt_avrcp_operation_id_t last_wear_action;
    bt_avrcp_operation_id_t operation_action;
    bt_avrcp_operation_state_t operation_state;
    uint32_t sdpc_service_handle;
    uint32_t avrcp_volume;
    bt_sink_srv_avrcp_get_folder_items_parameter_t folder_parameter;

    bt_avrcp_role_t avrcp_role;//[TD-PLAYER]
} bt_sink_srv_music_device_t;

//MTK_Titan: Add to support feeding media data
typedef struct _media_node_t {
    uint8_t *pData;
    uint32_t size;
    struct _media_node_t *pNext;
} media_node_t;

typedef struct {
    media_node_t *pHead; //data read from head
    media_node_t *pTail; //data write from tail
    uint32_t med_size;   //total queued size
    uint32_t node_cnt;   //total queued number
    bt_sink_srv_music_device_t *pRunDev;
} bt_sink_srv_music_data_list_t;
//(end)MTK_Titan: Add to support feeding media data

typedef struct {
    bt_sink_srv_music_state_t state;
    bt_sink_srv_music_device_t *run_dev;

    /* Audio manager ID */
    int8_t a2dp_aid;
    /* volume level */
    uint8_t vol_lev;
    uint8_t last_volume;
    uint8_t rho_flag;
    uint32_t init_sink_latency;
    uint8_t packet_count;
    bt_sink_srv_ctx_flag_t context_flag;

    bt_sink_srv_music_device_list_t played_connected_dev;

    bt_sink_srv_music_device_t sink_dev[BT_SINK_SRV_MUISC_DEV_COUNT];
    bt_sink_srv_music_device_t *latest_dev;
    bt_sink_srv_music_device_t *focus_dev;

    bt_sink_srv_music_data_list_t media_list; //MTK_Titan: Add to support feeding media data
} bt_sink_srv_music_context_t;

typedef struct {
    uint32_t frequence;
    uint32_t base_asi;
    uint32_t cur_asi;
    bt_clock_t *latency_clock;
    bt_clock_t *cur_recv_clock;
    bt_clock_t *pta;
} bt_sink_srv_music_optimize_pta_param_t;

typedef struct {
    uint32_t frequence;
    uint32_t gap_handle;
    bt_clock_t *pta;
    uint32_t packet_count;
    bt_sink_srv_music_data_info_t *packet_list;
} bt_sink_srv_music_set_sta_param_t;

typedef struct {
    uint16_t    music_volume;
    uint8_t     music_mode;
    uint32_t    play_order_count;
    uint8_t     local_asi_flag;
} bt_sink_srv_music_stored_data_t;

typedef struct {
    bt_bd_addr_t remote_addr;
    bt_sink_srv_music_stored_data_t dev_db;
} bt_sink_srv_music_data_list;

typedef struct {
    uint8_t volume;
    uint8_t volume_type;
} bt_sink_srv_audio_sync_music_data_t;

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

void bt_sink_srv_music_init(void);

void bt_sink_srv_music_init_context(void);

bt_sink_srv_music_context_t *bt_sink_srv_music_get_context(void);

bt_sink_srv_music_device_t *bt_sink_srv_music_get_device(bt_sink_srv_music_device_type_t type, const void *param);

void bt_sink_srv_music_reset_device(bt_sink_srv_music_device_t *dev);

void bt_sink_srv_music_set_focus_device(bt_sink_srv_music_device_t *dev);

void bt_sink_srv_music_update_run_device(bt_sink_srv_music_device_t *dev);

void bt_sink_srv_music_state_machine_handle(bt_sink_srv_music_device_t *dev, uint32_t evt_id, void *param);

void BT_A2DP_MAKE_SBC_CODEC(bt_a2dp_codec_capability_t *codec,
                            bt_a2dp_role_t role,
                            uint8_t min_bit_pool, uint8_t max_bit_pool,
                            uint8_t block_length, uint8_t subband_num,
                            uint8_t alloc_method, uint8_t sample_rate,
                            uint8_t channel_mode);

void BT_A2DP_MAKE_AAC_CODEC(bt_a2dp_codec_capability_t *codec,
                            bt_a2dp_role_t role, bool vbr, uint8_t object_type,
                            uint8_t channels, uint16_t sample_rate,
                            uint32_t bit_rate);

void BT_A2DP_MAKE_VENDOR_CODEC(bt_a2dp_codec_capability_t *codec,
                               bt_a2dp_role_t role, uint32_t vendor_id, uint16_t
                               codec_id, uint8_t sample_frequency, uint8_t
                               channel_mode);

void BT_A2DP_CONVERT_SBC_CODEC(bt_codec_capability_t *dst_codec,
                               bt_a2dp_codec_capability_t *src_codec);

void BT_A2DP_CONVERT_AAC_CODEC(bt_codec_capability_t *dst_codec,
                               bt_a2dp_codec_capability_t *src_codec);

void BT_A2DP_CONVERT_VENDOR_CODEC(bt_codec_capability_t *dst_codec,
                                  bt_a2dp_codec_capability_t *src_codec);

void bt_sink_srv_music_fill_audio_src_callback(audio_src_srv_handle_t *handle);

uint64_t bt_sink_srv_music_convert_btaddr_to_devid(bt_bd_addr_t *bd_addr);

uint8_t bt_sink_srv_get_vol_local2bt(uint8_t vol, uint8_t local_level, uint8_t bt_level);

uint8_t bt_sink_srv_get_vol_bt2local(uint8_t vol, uint8_t local_level, uint8_t bt_level);

void bt_sink_srv_music_drv_play(void *param);
void bt_sink_srv_music_drv_stop(bt_sink_srv_music_device_t *dev, uint8_t aid);
void bt_sink_srv_music_fill_am_aud_param(bt_sink_srv_am_audio_capability_t  *aud_cap,
                                         bt_a2dp_codec_capability_t *a2dp_cap, bt_a2dp_role_t role, uint16_t a2dp_mtu);
void bt_sink_srv_music_update_base_parameters_to_dsp(int16_t drift_value, uint32_t nclk, uint32_t asi_base, uint32_t asi_cur);
void bt_sink_srv_music_reject_a2dp_1M(void);
void bt_sink_srv_music_decide_play_pause_action(bt_sink_srv_music_device_t *dev);
void bt_sink_srv_music_avrcp_status_change_notify(bt_bd_addr_t *remote_addr, bt_avrcp_status_t avrcp_status);

/**
 * @brief                           Function to set sink latency, if sink latency is set, it would be valid after next time to start playing.
 * @param[in] latency_us    Is the sink latency value,it`s unit is us, and it should be the integer multiple of 1250us, default value is 140000us.
 * @return                         The status to set sink latency, if return value is not BT_STATUS_SUCCESS, please check the parameter.
 */
bt_status_t bt_sink_srv_music_set_sink_latency(uint32_t latency_value, bool is_initial_value);

/**
 * @brief                           Function to get sink latency.
 * @return                         The current sink latency value.
 */
uint32_t bt_sink_srv_music_get_sink_latency(void);

bt_status_t bt_sink_srv_music_get_nvdm_data(bt_bd_addr_t *remote_addr, bt_sink_srv_music_data_type_t data_type, void *data);

bt_status_t bt_sink_srv_music_set_nvdm_data(bt_bd_addr_t *remote_addr, bt_sink_srv_music_data_type_t data_type, void *data);

uint32_t bt_sink_srv_get_latency(void);

void bt_sink_srv_music_set_max_bit_pool(uint32_t max_bp);

void bt_sink_srv_update_last_device(bt_bd_addr_t *remote_addr, bt_sink_srv_profile_type_t module_type);

bt_avrcp_operation_id_t bt_sink_srv_music_get_play_pause_action(bt_sink_srv_music_device_t *dev);

uint32_t bt_sink_srv_music_find_free_timer(uint32_t start_timer_id, uint32_t end_timer_id);

bool bt_sink_srv_music_get_a2dp_nvdm_data(bt_bd_addr_t *bt_addr, void *data_p, uint32_t size);

bool bt_sink_srv_music_set_a2dp_nvdm_data(bt_bd_addr_t *bt_addr, void *data_p, uint32_t size);

void bt_sink_srv_music_set_ALC_enable(uint8_t is_enable);

uint8_t bt_sink_srv_music_get_ALC_enable(void);

void bt_sink_srv_music_device_waiting_list_operation(uint32_t *device_list, uint32_t device_count, bool is_add);

bt_sink_srv_music_playback_state_t bt_sink_srv_get_music_state(void);

bt_status_t bt_sink_srv_music_audio_switch(bool switch_on);
void bt_sink_srv_music_stop(bt_sink_srv_music_device_t *dev, uint8_t aid);
void bt_sink_srv_music_set_interrupt_flag(uint32_t flagMask, bool val);
bt_sink_srv_state_t bt_sink_srv_music_get_music_state(bt_bd_addr_t *addr);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifndef __BT_SINK_SRV_MUSIC_H__ */

