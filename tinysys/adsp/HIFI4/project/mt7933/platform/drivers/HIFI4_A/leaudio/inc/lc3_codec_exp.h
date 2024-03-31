#ifndef _LC3_CODEC_EXP_H_
#define _LC3_CODEC_EXP_H_

#define FRAUNHOFER_LC3_SUPPORT
#ifdef FRAUNHOFER_LC3_SUPPORT
#include "defines_global.h"
#include "lc3_dec.h"
#include "lc3_enc.h"
#include "audio_leaudio_type.h"
#else
#include <stdint.h>
#endif

typedef char           Word8;
typedef short          Word16;
typedef int            Word32;
typedef unsigned int   UWord32;

#define LC3_SET_PARAM_FAIL (-98)

#ifndef NULL
#define NULL 0
#endif

typedef struct lc3_dec_struct {
    unsigned int sampleRate;
    short        nChannels;
    int          bipsIn;
    int          bitrate;
    int          frame_dms;   // Note it should be 100 or 75
    int          plcMeth;
    int          nSamples;
    int          nBytes;
    void         *scratch;
    int          scratch_size;
    void         *buffer;
#ifdef FRAUNHOFER_LC3_SUPPORT
    LC3_Dec      *decoder;
#endif
} lc3_dec_t;

typedef struct lc3_enc_struct {
    unsigned int sampleRate;
    short        nChannels;
    int          bipsIn;
    int          bitrate;
    int          frame_dms;   // Note it should be 100 or 75
    int          nSamples;
    int          nBytes;
    void         *scratch;
    int          scratch_size;
#ifdef FRAUNHOFER_LC3_SUPPORT
    LC3_Enc      *encoder;
#endif
} lc3_enc_t;

#ifndef FRAUNHOFER_LC3_SUPPORT
typedef enum {
    LC3_OK,           /*!< No error occurred */
    LC3_DECODE_ERROR, /*!< Frame failed to decode and was concealed */
} LC3_ErrorCode;
#endif

LC3_ErrorCode LC3_ENC_Process(
    Word8  *pHandle,  //handle
    void  **pInBuf,   //input Samples
    Word8  *pOutBuf,  //MSBC packet
    Word32 *pOutLen   //output Length (byte)
);

LC3_ErrorCode LC3_DEC_Process(
    Word8  *pHandle,  //handle
    char   *pInBuf,   //input MSBC packet
    void  **pOutBuf,  //output Sample
    int  bfi_ext   //bfi flag
);

int LC3_ENC_SetParam(Word8 *pBuffer,
                     unsigned int sampleRate,
                     int frame_dms,
                     int bitrate,
                     short nChannels,
                     int bitDepth
                    );

int LC3_DEC_SetParam(Word8 *pBuffer,
                     unsigned int sampleRate,
                     int frame_dms,
                     int bitrate,
                     short nChannels,
                     int bitDepth,
                     int plcMeth
                    );


void LC3_Dec_Get_Param(Word8 *pBuffer,
                       int *decoder_size,
                       int *scratch_size,
                       int16_t *delay
                      );

void LC3_Enc_Get_Param(Word8 *pBuffer,
                       int *encoder_size,
                       int *scratch_size
                      );

int LC3_DEC_GetBufferSize( void );

int LC3_ENC_GetBufferSize( void );

void *LC3_DEC_Init(char *pBuffer );

void *LC3_ENC_Init(char *pBuffer );

void LC3_DEC_UnInit(char *pBuffer );

void LC3_ENC_UnInit(char *pBuffer );

#endif
