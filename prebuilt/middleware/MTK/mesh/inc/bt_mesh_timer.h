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

#ifndef MESH_TIMER_H
#define MESH_TIMER_H
#include <stdbool.h>
#include <stdint.h>
#include "bt_mesh_timer_definition_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MESH_TIMER_STATUS_SUCCESS  0
#define MESH_TIMER_STATUS_FAIL     -1
#define MESH_TIMER_STATUS_OOM      -2
typedef int32_t  mesh_timer_status_t;

/**
 * @brief                  Timeout callback function prototype
 * @param[in] timer_id     Timer ID
 * @param[in] data         User data saved in timer instance
 * @return                 None
 */
typedef void (*mesh_timeout_callback_t) (uint32_t data);

/**
 * @brief                           Timeout instance structure
 */
typedef struct _mesh_timer_t {
    uint32_t timer_id;                /**<  SM id + SM defined id */
    uint32_t data;                    /**<  user data */
    uint32_t time_tick;               /**<  timer timeout in tick */
    mesh_timeout_callback_t cb;       /**<  timer timeout callback function */
} mesh_timer_t;

/**
 * @brief                   To start a timer
 * @param[in] timer_id      Timer ID
 * @param[in] data          User data saved in timer instance
 * @param[in] time_ms       timer timeout in ms
 * @param[in] cb            timer timeout callback function
 * @return                  MESH_TIMER_STATUS_SUCCESS if add timer success
                            MESH_TIMER_STATUS_OOM if timer reach max count
                            MESH_TIMER_STATUS_FAIL if double start
 */
mesh_timer_status_t mesh_timer_start(uint32_t timer_id, uint32_t data, uint32_t time_ms, mesh_timeout_callback_t cb);

/**
 * @brief                   Stop a timer
 * @param[in] timer_id      Timer ID
 * @return                  MESH_TIMER_STATUS_SUCCESS if cancel timer success
                            MESH_TIMER_STATUS_FAIL if not found
 */
mesh_timer_status_t mesh_timer_stop(uint32_t timer_id);

/**
 * @brief                   Stop a timer with return user data.
 * @param[in] timer_id      Timer ID
 * @return                  user data of timer, or NULL if not found
 */
void *mesh_timer_stop_with_user_data(uint32_t timer_id);

void mesh_timer_check_status(uint32_t is_interrupt);

/**
 * @brief                   Find a timer
 * @param[in] timer_id      Timer ID
 * @return                  A pointer to the timer instance
 */
mesh_timer_t *mesh_timer_find(uint32_t timer_id);

void mesh_timer_init(void);
void mesh_timer_deinit(void);


#ifdef __cplusplus
}
#endif

#endif /* MESH_TIMER_H */

