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


#ifndef __BT_MESH_MODEL_TIME_OPCODES_H__
#define __BT_MESH_MODEL_TIME_OPCODES_H__

/**
 *   @addtogroup BluetoothMesh Mesh
 *   @{
 *   @addtogroup BluetoothMeshSigModel Sig_Model
 *   @{
 *   @addtogroup BluetoothMeshTimeModel Time_Model
 *   @{
 *     bt_mesh_model_time_opcodes.h defines the SIG Mesh Time Model operation codes.
*/

/**
 * @defgroup Bluetooth_mesh_time_model_define Define
 * @{
*/

/*!
     @name Time model message opcode.
     @brief Opcode for time models.
     @{
 */
#define BT_MESH_MODEL_TIME_GET                                           0X8237
#define BT_MESH_MODEL_TIME_SET                                           0X5C
#define BT_MESH_MODEL_TIME_STATUS                                        0X5D
#define BT_MESH_MODEL_TIME_ROLE_GET                                      0X8238
#define BT_MESH_MODEL_TIME_ROLE_SET                                      0X8239
#define BT_MESH_MODEL_TIME_ROLE_STATUS                                   0X823A
#define BT_MESH_MODEL_TIME_ZONE_GET                                      0X823B
#define BT_MESH_MODEL_TIME_ZONE_SET                                      0X823C
#define BT_MESH_MODEL_TIME_ZONE_STATUS                                   0X823D
#define BT_MESH_MODEL_TIME_TAI_UTC_DELTA_GET                             0X823E
#define BT_MESH_MODEL_TIME_TAI_UTC_DELTA_SET                             0X823F
#define BT_MESH_MODEL_TIME_TAI_UTC_DELTA_STATUS                          0X8240
/*!  @} */

/*!
@}
@}
@}
@}
*/

#endif // __BT_MESH_MODEL_TIME_OPCODES_H__

