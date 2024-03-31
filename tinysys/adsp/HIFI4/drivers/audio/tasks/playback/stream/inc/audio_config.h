/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */


#ifndef __AUDIO_CONFIG_H__
#define __AUDIO_CONFIG_H__

/*!
  @brief audio_hardware used in StreamAudioSource()/StreamAudioSink() traps.
*/

typedef enum {
    AUDIO_HARDWARE_PCM, /*!< The audio PCM hardware with high audio quality. "instance" specifies the physical interface (AUDIO_INSTANCE_0 is the first PCM interface, AUDIO_INSTANCE_1 is the second, on chips which have two).
                         Audio hardware PCM, I2S and SPDIF are mutually exclusive for the same audio_instance.
                         "channel" specifies the PCM slot; only AUDIO_CHANNEL_SLOT_x are valid for this hardware type.*/
    AUDIO_HARDWARE_I2S_M, /*!< The audio I2S hardware. "instance" specifies the physical interface (AUDIO_INSTANCE_0 is the first I2S interface, AUDIO_INSTANCE_1 is the second, on chips which have two).
                         Audio hardware PCM, I2S and SPDIF are mutually exclusive for the same audio_instance.
                         "channel" specifies the I2S slot; only AUDIO_CHANNEL_SLOT_x are valid for this hardware type.*/
    AUDIO_HARDWARE_I2S_S,
#ifdef SPDIF_SUPPORT    
    AUDIO_HARDWARE_SPDIF, /*!< The audio SPDIF hardware. "instance" specifies the physical interface (AUDIO_INSTANCE_0 is the first SPDIF interface, AUDIO_INSTANCE_1 is the second, on chips which have two).
                           Audio hardware PCM, I2S and SPDIF are mutually exclusive for the same audio_instance.
                           "channel" specifies the SPDIF slot; only AUDIO_CHANNEL_SLOT_x are valid for this hardware type.*/
#endif /* SPDIF_SUPPORT */                           
    AUDIO_HARDWARE_CODEC, /*!< The audio CODEC hardware. "instance" specifies the physical interface (AUDIO_INSTANCE_0 is the first CODEC interface, AUDIO_INSTANCE_1 is the second, on chips which have two).
                           "channel" specifies the CODEC channel; only AUDIO_CHANNEL_A or AUDIO_CHANNEL_B or AUDIO_CHANNEL_A_AND_B are valid for this hardware type.
                           On chips with stereo CODECs, an "instance" consists of a pair of channels (stereo).*/
    AUDIO_HARDWARE_DIGITAL_MIC, /*!< The audio digital MIC hardware. "instance" specifies the physical interface (AUDIO_INSTANCE_0 is the first digital MIC interface, AUDIO_INSTANCE_1 is the second,
                                     AUDIO_INSTANCE_2 is the third, on chips which have three).
                                     "channel" specifies the digital MIC channel; only AUDIO_CHANNEL_A or AUDIO_CHANNEL_B are valid for this hardware type.
                                     An "instance" consists of a pair of channels (stereo).*/
#ifdef MINI_DSP_SUPPORT
    AUDIO_HARDWARE_MINI_DSP,
#endif /* MINI_DSP_SUPPORT */    
} audio_hardware;

typedef enum {
    INSTANCE_A, //"instance A" specifies the physical interface A for given audio hardware
    INSTANCE_B, //"instance B" specifies the physical interface B for given audio hardware
    INSTANCE_C, //"instance C" specifies the physical interface C for given audio hardware
    INSTANCE_D, //"instance D" specifies the physical interface D for given audio hardware
} audio_instance;


/*!
  @brief audio_channel used in StreamAudioSource(), StreamAudioSink() and CodecSetIirFilter() traps.
*/

typedef enum {
    AUDIO_CHANNEL_A, /*!< The audio channel A*/
    AUDIO_CHANNEL_B, /*!< The audio channel B*/
    AUDIO_CHANNEL_A_AND_B, /*!< The audio channel A and B (obtaining the stereo CODEC in mono-mode)*/
    AUDIO_CHANNEL_3ch,
    AUDIO_CHANNEL_4ch,
    AUDIO_CHANNEL_5ch,
    AUDIO_CHANNEL_6ch,
    AUDIO_CHANNEL_VP,
#ifdef AIR_AUDIO_I2S_SLAVE_TDM_ENABLE
    AUDIO_CHANNEL_8ch,
#endif
    AUDIO_CHANNEL_SLOT_0 = 0, /*!< The audio digital slot 0*/
    AUDIO_CHANNEL_SLOT_1, /*!< The audio digital slot 1*/
    AUDIO_CHANNEL_SLOT_2, /*!< The audio digital slot 2*/
    AUDIO_CHANNEL_SLOT_3, /*!< The audio digital slot 3*/
    SPDIF_CHANNEL_A = 0, /*!< The 1st SPDIF subframe*/
    SPDIF_CHANNEL_B, /*!< The 2nd SPDIF subframe*/
    SPDIF_CHANNEL_USER, /*!< The User data in SPDIF subframes*/
    SPDIF_CHANNEL_A_B_INTERLEAVED, /*!< SPDIF channels to be interleaved*/
    AUDIO_BRANCH_A_AND_B,
    AUDIO_JOINT_A_AND_B,
} audio_channel;

typedef enum {
    RT_mode,
    VP_amr59,
    VP_amr122,
    VP_WB,
    VP_ADPCM,
    VP_mSBC,
} PATTERN_CODEC;

typedef enum {
    CVSD = 0,
    mSBC = 1,
    TRANSPARENT = 2,
} SCO_CODEC;

typedef enum {
    SCO  = 0,
    eSCO = 1,
} SCO_type;

typedef enum {
    TxOut = 0,
    RxIn  = 1,
} SCO_INOUT;

typedef enum {
    EV3  = 0,
    EV3_2 = 1,
    EV4  = 2,
    EV5  = 3,
} SCO_PACKET_type;
typedef enum {
    Flash_addr  = 0,
    Ram_addr = 1,
} Memory_type;

#endif  /* __APP_AUDIO_IF_H__ */
