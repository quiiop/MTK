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

 
#ifndef _DSP_TEMP_H_
#define _DSP_TEMP_H_

#include "audio_types.h"
#include <mt_printf.h>
#include <string.h>

#ifdef DSP_USE_MSGID_SEND_LOG
#define DSP_MW_LOG_E(_message, arg_cnt, ...)  LOG_MSGID_E(dsp_mw,_message, arg_cnt, ##__VA_ARGS__)
#define DSP_MW_LOG_W(_message, arg_cnt, ...)  LOG_MSGID_W(dsp_mw,_message, arg_cnt, ##__VA_ARGS__)
#define DSP_MW_LOG_I(_message, arg_cnt, ...)  LOG_MSGID_I(dsp_mw,_message, arg_cnt, ##__VA_ARGS__)
#define DSP_MW_LOG_D(_message, arg_cnt, ...)  LOG_MSGID_D(dsp_mw,_message, arg_cnt, ##__VA_ARGS__)
#else
#define DSP_MW_LOG_E(_message, arg_cnt, ...)  PRINTF_E(_message, ##__VA_ARGS__)
#define DSP_MW_LOG_W(_message, arg_cnt, ...)  PRINTF_W(_message, ##__VA_ARGS__) 
#define DSP_MW_LOG_I(_message, arg_cnt, ...)  PRINTF_I(_message, ##__VA_ARGS__) 
#define DSP_MW_LOG_D(_message, arg_cnt, ...)  PRINTF_D(_message, ##__VA_ARGS__) 
#endif

#ifndef AUDIO_HAL_READY
typedef enum {
    HAL_AUDIO_DEVICE_NONE               = 0x0000,  /**<  No audio device is on. */
    HAL_AUDIO_DEVICE_MAIN_MIC_L         = 0x0001,  /**<  Stream in: main mic L. */
    HAL_AUDIO_DEVICE_MAIN_MIC_R         = 0x0002,  /**<  Stream in: main mic R. */
    HAL_AUDIO_DEVICE_MAIN_MIC_DUAL      = 0x0003,  /**<  Stream in: main mic L+R. */
    HAL_AUDIO_DEVICE_LINEINPLAYBACK_L   = 0x0004,  /**<  Stream in: line in playback L. */
    HAL_AUDIO_DEVICE_LINEINPLAYBACK_R   = 0x0008,  /**<  Stream in: line in playback R. */
    HAL_AUDIO_DEVICE_LINEINPLAYBACK_DUAL= 0x000c,  /**<  Stream in: line in playback L+R. */
    HAL_AUDIO_DEVICE_DIGITAL_MIC_L      = 0x0010,  /**<  Stream in: digital mic L. */
    HAL_AUDIO_DEVICE_DIGITAL_MIC_R      = 0x0020,  /**<  Stream in: digital mic R. */
    HAL_AUDIO_DEVICE_DIGITAL_MIC_DUAL   = 0x0030,  /**<  Stream in: digital mic L+R. */

    HAL_AUDIO_DEVICE_DAC_L              = 0x0100,  /**<  Stream out:speaker L. */
    HAL_AUDIO_DEVICE_DAC_R              = 0x0200,  /**<  Stream out:speaker R. */
    HAL_AUDIO_DEVICE_DAC_DUAL           = 0x0300,  /**<  Stream out:speaker L+R. */

    HAL_AUDIO_DEVICE_I2S_MASTER         = 0x1000,  /**<  Stream in/out: I2S master role */
    HAL_AUDIO_DEVICE_I2S_SLAVE          = 0x2000,  /**<  Stream in/out: I2S slave role */
    HAL_AUDIO_DEVICE_EXT_CODEC          = 0x3000,   /**<  Stream out: external amp.&codec, stereo/mono */

    HAL_AUDIO_DEVICE_MAIN_MIC           = 0x0001,       /**<  OLD: Stream in: main mic. */
    HAL_AUDIO_DEVICE_HEADSET_MIC        = 0x0002,       /**<  OLD: Stream in: earphone mic. */
    HAL_AUDIO_DEVICE_HANDSET            = 0x0004,       /**<  OLD: Stream out:receiver. */
    HAL_AUDIO_DEVICE_HANDS_FREE_MONO    = 0x0008,       /**<  OLD: Stream out:loudspeaker, mono. */
    HAL_AUDIO_DEVICE_HANDS_FREE_STEREO  = 0x0010,       /**<  OLD: Stream out:loudspeaker, stereo to mono L=R=(R+L)/2. */
    HAL_AUDIO_DEVICE_HEADSET            = 0x0020,       /**<  OLD: Stream out:earphone, stereo */
    HAL_AUDIO_DEVICE_HEADSET_MONO       = 0x0040,       /**<  OLD: Stream out:earphone, mono to stereo. L=R. */
    HAL_AUDIO_DEVICE_LINE_IN            = 0x0080,       /**<  OLD: Stream in/out: line in. */
    HAL_AUDIO_DEVICE_DUAL_DIGITAL_MIC   = 0x0100,       /**<  OLD: Stream in: dual digital mic. */
    HAL_AUDIO_DEVICE_SINGLE_DIGITAL_MIC = 0x0200,       /**<  OLD: Stream in: single digital mic. */

    HAL_AUDIO_DEVICE_DUMMY              = 0xFFFFFFFF,   /**<  for DSP structrue alignment */
} hal_audio_device_t;

typedef enum {
    HAL_AUDIO_DIRECT                     = 0, /**< A single interconnection, output equal to input. */
    HAL_AUDIO_SWAP_L_R                   = 2, /**< L and R channels are swapped. That is (L, R) -> (R, L). */
    HAL_AUDIO_BOTH_L                     = 3, /**< only output L channel. That is (L, R) -> (L, L). */
    HAL_AUDIO_BOTH_R                     = 4, /**< only output R channel. That is (L, R) -> (R, R). */
    HAL_AUDIO_MIX_L_R                    = 5, /**< L and R channels are mixed. That is (L, R) -> (L+R, L+R). */
    HAL_AUDIO_MIX_SHIFT_L_R              = 6, /**< L and R channels are mixed and shift. That is (L, R) -> (L/2+R/2, L/2+R/2). */
    HAL_AUDIO_CHANNEL_DUMMY              = 0xFFFFFFFF,   /**<  for DSP structrue alignment */
} hal_audio_channel_selection_t;

typedef enum {
    HAL_AUDIO_MEM1                              = 0x0001,       /**< Memory path 1. UL:DL1_data, VUL:VUL1_data   */
    HAL_AUDIO_MEM2                              = 0x0002,       /**< Memory path 2. UL:DL2_data, VUL:VUL2_data   */
    HAL_AUDIO_MEM3                              = 0x0004,       /**< Memory path 3. UL:DL3_data, VUL:AWB_data   */
    HAL_AUDIO_MEM4                              = 0x0008,       /**< Memory path 4. UL:DL12_data, VUL:AWB2_data   */
    HAL_AUDIO_MEM5                              = 0x0010,
    HAL_AUDIO_MEM6                              = 0x0020,
    HAL_AUDIO_MEM7                              = 0x0040,
    HAL_AUDIO_MEM_SUB                           = 0x0080,
    HAL_AUDIO_MEM_DUMMY                         = 0xFFFFFFFF,   /**<  for DSP structrue alignment */
} hal_audio_memory_t;

typedef enum {
    HAL_AUDIO_INTERFACE_NONE                    = 0x0000,       /**< No audio interface should be selected, */
    HAL_AUDIO_INTERFACE_1                       = 0x0001,       /**< Audio interface path 1. UL:UL SRC ch1 ch2, I2S Master:I2S0, */
    HAL_AUDIO_INTERFACE_2                       = 0x0002,       /**< Audio interface path 2. UL:UL SRC ch3 ch4, I2S Master:I2S1, */
    HAL_AUDIO_INTERFACE_3                       = 0x0004,       /**< Audio interface path 3. UL:UL SRC ch5 ch6, I2S Master:I2S2, */
    HAL_AUDIO_INTERFACE_4                       = 0x0008,       /**< Audio interface path 4. I2S Master:I2S3, */
    HAL_AUDIO_INTERFACE_DUMMY                   = 0xFFFFFFFF,   /**<  for DSP structrue alignment */
} hal_audio_interface_t;

/** @brief audio channel number define */
typedef enum {
    HAL_AUDIO_MONO                  = 0, /**< A single channel.  */
    HAL_AUDIO_STEREO                = 1, /**< Two channels. */
    HAL_AUDIO_STEREO_BOTH_L_CHANNEL = 2, /**< Two channels, but only output L channel. That is (L, R) -> (L, L). */
    HAL_AUDIO_STEREO_BOTH_R_CHANNEL = 3, /**< Two channels, but only output R channel. That is (L, R) -> (R, R). */
    HAL_AUDIO_STEREO_BOTH_L_R_SWAP  = 4  /**< Two channels, L and R channels are swapped. That is (L, R) -> (R, L). */
} hal_audio_channel_number_t;

typedef struct {
    uint32_t phys_buffer_addr;
    //uint8_t  *pSramBufAddr;
    uint32_t u4BufferSize;
    uint32_t u4asrc_buffer_size;/* asrc output buffer size      */
    int32_t  u4DataRemained;
    uint32_t u4SampleNumMask;    /* sample number mask */
    uint32_t u4SamplesPerInt;    /* number of samples to play before interrupting */
    int32_t  u4WriteIdx;         /* Previous Write Index. */
    int32_t  u4ReadIdx;          /* Previous Read Index. */
    uint32_t u4MaxCopySize;
    BOOL     u4awsflag;          /* Indicate AWS is on or not*/
    BOOL     u4asrcflag;         /* Indicate ASRC is on or not**/
    uint32_t u4asrcrate;         /* sample rate of ASRC out */
} afe_block_t;

#define ATTR_ALIGN(alignment) __attribute__ ((__aligned__(alignment)))
#define ATTR_SECTION_W_LINE_1(sec, line) __attribute__ ((__section__(sec #line)))
#define ATTR_SECTION_W_LINE(sec, line) ATTR_SECTION_W_LINE_1(sec, line)

//#define ATTR_TEXT_IN_IRAM              ATTR_SECTION_W_LINE(".iram.", __LINE__)
#define ATTR_TEXT_IN_IRAM
#define ATTR_RWDATA_IN_DRAM            __attribute__ ((__section__(".data")))
#define ATTR_ZIDATA_IN_DRAM            __attribute__ ((__section__(".bss")))
#define ATTR_RWDATA_IN_DRAM_4BYTE_ALIGN    __attribute__ ((__section__(".data"),__aligned__(4)))
#define ATTR_ZIDATA_IN_DRAM_4BYTE_ALIGN    __attribute__ ((__section__(".bss"),__aligned__(4)))
#define ATTR_RWDATA_IN_DRAM_8BYTE_ALIGN    __attribute__ ((__section__(".data"),__aligned__(8)))
#define ATTR_ZIDATA_IN_DRAM_8BYTE_ALIGN    __attribute__ ((__section__(".bss"),__aligned__(8)))
#define ATTR_RWDATA_IN_DRAM_16BYTE_ALIGN    __attribute__ ((__section__(".data"),__aligned__(16)))
#define ATTR_ZIDATA_IN_DRAM_16BYTE_ALIGN    __attribute__ ((__section__(".bss"),__aligned__(16)))

#ifndef MTK_LOWPOWER_LEVEL
#define MTK_LOWPOWER_LEVEL 0
#endif
#if (MTK_LOWPOWER_LEVEL == 0)
#define ATTR_TEXT_IN_IRAM_LEVEL_1
#define ATTR_TEXT_IN_IRAM_LEVEL_2
#elif (MTK_LOWPOWER_LEVEL == 1)
#define ATTR_TEXT_IN_IRAM_LEVEL_1   ATTR_TEXT_IN_IRAM
#define ATTR_TEXT_IN_IRAM_LEVEL_2
#elif (MTK_LOWPOWER_LEVEL == 2)
#define ATTR_TEXT_IN_IRAM_LEVEL_1   ATTR_TEXT_IN_IRAM
#define ATTR_TEXT_IN_IRAM_LEVEL_2   ATTR_TEXT_IN_IRAM
#else
#define ATTR_TEXT_IN_IRAM_LEVEL_1
#define ATTR_TEXT_IN_IRAM_LEVEL_2
#endif

typedef enum {
    AFE_PCM_FORMAT_S8 = 0,
    AFE_PCM_FORMAT_U8,
    AFE_PCM_FORMAT_S16_LE,
    AFE_PCM_FORMAT_S16_BE,
    AFE_PCM_FORMAT_U16_LE,
    AFE_PCM_FORMAT_U16_BE,
    AFE_PCM_FORMAT_S24_LE,
    AFE_PCM_FORMAT_S24_BE,
    AFE_PCM_FORMAT_U24_LE,
    AFE_PCM_FORMAT_U24_BE,
    AFE_PCM_FORMAT_S32_LE,
    AFE_PCM_FORMAT_S32_BE,
    AFE_PCM_FORMAT_U32_LE,
    AFE_PCM_FORMAT_U32_BE,
    AFE_PCM_FORMAT_LAST,
    AFE_PCM_FORMAT_DUMMY  = 0xFFFFFFFF,   /**<  for DSP structrue alignment */
} afe_pcm_format_t;


#endif



/******************************************************************************
 * External Global Variables
 ******************************************************************************/


/******************************************************************************
 * External Function Prototypes
 ******************************************************************************/
extern VOID Audio_Setup(VOID);
extern VOID DSP_SetDefaultAfePara(VOID);


/******************************************************************************
 * Inline Functions
 ******************************************************************************/




#endif /* _DSP_TEMP_H_ */

