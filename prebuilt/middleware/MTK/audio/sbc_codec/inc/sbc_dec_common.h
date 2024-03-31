/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2013
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

/*******************************************************************************
 *
 * Filename:
 * ---------
 * sbc_dec_common.h
 *
 * Project:
 * --------
 * SWIP
 *
 * Description:
 * ------------
 * SBC decoder common define header
 *
 * Author:
 * -------
 * Scholar Chang
 *
 *------------------------------------------------------------------------------
 * $Revision:  $
 * $Modtime:   $
 * $Log:       $
 *
 * 08 09 2015 scholar.chang
 * [WCPSP00000754] [Audio][SWIP]SBC codec SWIP check-in
 * Replace CRC table.
 *
 * 08 09 2015 scholar.chang
 * [WCPSP00000754] [Audio][SWIP]SBC codec SWIP check-in
 * Support mSBC decode.
 *
 * 10 20 2014 scholar.chang
 * [WCPSP00000712] [Audio][SWIP][SBC Decoder]SBC Decoder and Decoder Parser Check-In
 * Add silence frame.
 *
 * 04 11 2014 wendy.lin
 * [WCPSP00000733] SBC decoder optimize
 *  Optimize SBC Decoder.
 *  oritnial MCPS on target 57 MCPS => 24MCPS
 *
 *
 *******************************************************************************/

#ifndef __SBC_DEC_COMMON_H__
#define __SBC_DEC_COMMON_H__

#include "sbc_decoder.h"

#ifndef NULL
#define NULL 0
#endif

typedef unsigned char   U8;
typedef unsigned short  U16;
typedef unsigned int    U32;
typedef signed char     S8;
typedef signed short    S16;
typedef signed int      S32;

#if defined(__GNUC__)
typedef long long       S64;
typedef long long       U64;
#else   /* defined(__GNUC__) */
typedef __int64         S64;
typedef __int64         U64;
#endif  /* defined(__GNUC__) */

#define SBC_SYNCWORD    0x9C
#define mSBC_SYNCWORD   0xAD

// sampling frequency
#define SBC_FREQ_16000      0x00
#define SBC_FREQ_32000      0x01
#define SBC_FREQ_44100      0x02
#define SBC_FREQ_48000      0x03

// blocks
#define SBC_BLK_4       0x00
#define SBC_BLK_8       0x01
#define SBC_BLK_12      0x02
#define SBC_BLK_16      0x03

// subbands
#define SBC_SB_4        0x00
#define SBC_SB_8        0x01

// channel mode
#define SBC_MODE_MONO           0x00
#define SBC_MODE_DUAL_CHANNEL   0x01
#define SBC_MODE_STEREO         0x02
#define SBC_MODE_JOINT_STEREO   0x03

// allocation method
#define SBC_AM_LOUDNESS 0x00
#define SBC_AM_SNR      0x01

typedef struct {
    void *priv;
    unsigned char frequency;
    unsigned char blocks;
    unsigned char subbands;
    unsigned char mode;
    unsigned char allocation;
    unsigned char bitpool;
} SbcDec_Handle;

typedef struct {
    unsigned short magic_word;
    unsigned short prev_payload_size;
    unsigned char  prev_payload_header;
} SbcDec_MediaPayloadInfo;

#define fabs(x) ((x) < 0 ? -(x) : (x))
/* C does not provide an explicit arithmetic shift right but this will
   always be correct and every compiler *should* generate optimal code */

#define ASR(val, bits) ((int32_t)(val)) >> (bits)
#define ASL(val, bits) ((int32_t)(val)) << (bits)

#define SCALE_SPROTO4_TBL   12
#define SCALE_SPROTO8_TBL   14

#define SCALE_NPROTO4_TBL   12
#define SCALE_NPROTO8_TBL   12

#define SCALE4_STAGED1_BITS 15
#define SCALE4_STAGED2_BITS 16
#define SCALE8_STAGED1_BITS 15
#define SCALE8_STAGED2_BITS 16

typedef int32_t sbc_fixed_t;

#define SCALE4_STAGED1(src) ASR(src, SCALE4_STAGED1_BITS)
#define SCALE4_STAGED2(src) ASR(src, SCALE4_STAGED2_BITS)
#define SCALE8_STAGED1(src) ASR(src, SCALE8_STAGED1_BITS)
#define SCALE8_STAGED2(src) ASR(src, SCALE8_STAGED2_BITS)

#define SBC_FIXED_0(val) { val = 0; }
#define MUL(a, b)        ((a) * (b))
#define MULA(a, b, res)  ((a) * (b) + (res))
//#define MULA smlawb
//#define MUL smulwb
#define SCALE_OUT_BITS 15
#define SBC_X_BUFFER_SIZE 328

#define FIXED_A int32_t // data type for fixed point accumulator
#define FIXED_T int16_t // data type for fixed point constants
#define SBC_FIXED_EXTRA_BITS 0

#define CHAR_BIT        8

#define SBC_PROTO_FIXED4_SCALE \
    ((sizeof(FIXED_T) * CHAR_BIT - 1) - SBC_FIXED_EXTRA_BITS + 1)
#define F_PROTO4(x) (FIXED_A) ((x * 2) * \
    ((FIXED_A) 1 << (sizeof(FIXED_T) * CHAR_BIT - 1)) + 0.5)

#define SBC_COS_TABLE_FIXED4_SCALE \
    ((sizeof(FIXED_T) * CHAR_BIT - 1) + SBC_FIXED_EXTRA_BITS)
#define F_COS4(x) (FIXED_A) ((x) * \
    ((FIXED_A) 1 << (sizeof(FIXED_T) * CHAR_BIT - 1)) + 0.5)

#define SBC_PROTO_FIXED8_SCALE \
    ((sizeof(FIXED_T) * CHAR_BIT - 1) - SBC_FIXED_EXTRA_BITS + 1)
#define F_PROTO8(x) (FIXED_A) ((x * 2) * \
    ((FIXED_A) 1 << (sizeof(FIXED_T) * CHAR_BIT - 1)) + 0.5)

#define SBC_COS_TABLE_FIXED8_SCALE \
    ((sizeof(FIXED_T) * CHAR_BIT - 1) + SBC_FIXED_EXTRA_BITS)
#define F_COS8(x) (FIXED_A) ((x) * \
    ((FIXED_A) 1 << (sizeof(FIXED_T) * CHAR_BIT - 1)) + 0.5)

/*
 * Enforce 16 byte alignment for the data, which is supposed to be used
 * with SIMD optimized code.
 */
#define SBC_ALIGN_BITS 4
#define SBC_ALIGN_MASK ((1 << (SBC_ALIGN_BITS)) - 1)

/* This structure contains an unpacked SBC frame.
   Yes, there is probably quite some unused space herein */

#define SBC_MODE_SILENCE 0x4

typedef enum {
    MONO         = SBC_MODE_MONO,
    DUAL_CHANNEL = SBC_MODE_DUAL_CHANNEL,
    STEREO       = SBC_MODE_STEREO,
    JOINT_STEREO = SBC_MODE_JOINT_STEREO
} SBC_MODE;

typedef enum {
    LOUDNESS    = SBC_AM_LOUDNESS,
    SNR         = SBC_AM_SNR
} SBC_ALLOCATION;

struct sbc_frame {
    uint8_t frequency;
    uint8_t block_mode;
    uint8_t blocks;
    SBC_MODE mode;
    uint8_t channels;
    SBC_ALLOCATION allocation;
    uint8_t subband_mode;
    uint8_t subbands;
    uint8_t bitpool;
    uint8_t length;

    // bit number x set means joint stereo has been used in subband x
    uint8_t joint;

    // only the lower 4 bits of every element are to be used
    uint32_t scale_factor[2][8];

    // raw integer subband samples in the frame
    int32_t sb_sample_f[16][2][8];

    // modified subband samples
    int32_t sb_sample[16][2][8];

    // original pcm audio samples
    int16_t pcm_sample[16 * 8][2];
};

struct sbc_decoder_state {
    int subbands;
    int32_t V[2][170];
    int offset[2][16];
};

struct sbc_priv {
    int init;
    U8 p_table[256];
    struct sbc_frame frame;
    struct sbc_decoder_state dec_state;
};

int sbc_init(SbcDec_Handle *sbc, void *priv_alloc_base);

// Decodes ONE input block into ONE output block
int sbc_decode(
    SbcDec_Handle *sbc,
    const void *input,
    uint32_t input_len,
    void *output,
    uint32_t output_len,
    uint32_t *written);

int sbc_unpack_frame(
    const uint8_t *data,
    struct sbc_frame *frame,
    uint32_t len,
    U8 *p_table);

uint32_t sbc_get_output_size(struct sbc_frame *frame);

S32 sbc_get_frame_sample_count(const U8 *p_in, U32 *p_in_byte_cnt);

extern const int sbc_offset4[4][4];
extern const int sbc_offset8[4][8];
extern const int32_t sbc_proto_4_40m0[];
extern const int32_t sbc_proto_4_40m1[];
extern const int32_t sbc_proto_8_80m0[];
extern const int32_t sbc_proto_8_80m1[];

extern const int16_t synmatrix4[4][2];
extern const int16_t synmatrix8[7][4];

#endif  // __SBC_DEC_COMMON_H__
