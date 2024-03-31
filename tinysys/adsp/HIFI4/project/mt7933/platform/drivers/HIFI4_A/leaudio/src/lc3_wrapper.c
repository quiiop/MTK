#include "lc3_codec_exp.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "mtk_heap.h"
#include "le_main.h"

#define UNUSED(x) (void)(x)    /* silence unused parameter warning */

#define scratchAlign(ptr, offset) (void *)(((uintptr_t)(ptr) + (offset) + 0x7) & (uintptr_t)~0x7)

LE_SECTION_FUNC LC3_ErrorCode LC3_ENC_Process(
    Word8  *pHandle,  //handle
    void  **pInBuf,   //input Samples
    Word8  *pOutBuf,  //MSBC packet
    Word32 *pOutLen   //output Length (byte)
)
{
#ifdef FRAUNHOFER_LC3_SUPPORT
    LC3_ErrorCode err = LC3_OK;
    lc3_enc_t *lc3    = (lc3_enc_t *)pHandle;
    LC3_Enc *encoder  = lc3->encoder;
    void *scratch     = lc3->scratch;

    err = lc3_enc_process_frame(encoder, pInBuf, pOutBuf, pOutLen, scratch);
    return err;
#else
    UNUSED(pHandle);
    UNUSED(pInBuf);
    UNUSED(pOutBuf);
    UNUSED(pOutLen);
    printf("%s() dummy function, not implemented\n", __FUNCTION__);
    return LC3_OK;
#endif
}

LE_SECTION_FUNC LC3_ErrorCode LC3_DEC_Process(
    Word8  *pHandle,  //handle
    char   *pInBuf,   //input MSBC packet
    void  **pOutBuf,  //output Sample
    int     bfi_ext
)
{
#ifdef FRAUNHOFER_LC3_SUPPORT
    LC3_ErrorCode err = LC3_OK;
    lc3_dec_t *lc3    = (lc3_dec_t *)pHandle;
    LC3_Dec *decoder  = lc3->decoder;
    void *scratch     = lc3->scratch;

    err = lc3_dec_process_frame(decoder, pInBuf, lc3->nBytes, bfi_ext, pOutBuf, scratch);
    return err;
#else
    UNUSED(pHandle);
    UNUSED(pInBuf);
    UNUSED(pOutBuf);
    UNUSED(bfi_ext);
    printf("%s() dummy function, not implemented\n", __FUNCTION__);
    return LC3_OK;
#endif
}

LE_SECTION_FUNC int LC3_ENC_SetParam(Word8 *pBuffer,
                                     unsigned int sampleRate,
                                     int frame_dms,
                                     int bitrate,
                                     short nChannels,
                                     int bitDepth
                                    )
{
#ifdef FRAUNHOFER_LC3_SUPPORT
    lc3_enc_t *lc3 = (lc3_enc_t *)pBuffer;
    if (pBuffer == NULL || !(frame_dms == 75 || frame_dms == 100)) {
        printf("%s() Invalid parameter pBuffer(%p), frame_dms(%d)\n", __FUNCTION__, pBuffer, frame_dms);
        return LC3_SET_PARAM_FAIL;
    }
    lc3->sampleRate = sampleRate;
    lc3->frame_dms  = frame_dms;
    lc3->bitrate    = bitrate;
    lc3->nChannels  = nChannels;
    lc3->bipsIn     = bitDepth;
#else
    UNUSED(pBuffer);
    UNUSED(sampleRate);
    UNUSED(frame_dms);
    UNUSED(bitrate);
    UNUSED(nChannels);
    UNUSED(bitDepth);
    printf("%s() dummy function, not implemented\n", __FUNCTION__);
#endif
    return 0;
}

LE_SECTION_FUNC int LC3_DEC_SetParam(Word8 *pBuffer,
                                     unsigned int sampleRate,
                                     int frame_dms,
                                     int bitrate,
                                     short nChannels,
                                     int bitDepth,
                                     int plcMeth
                                    )
{
#ifdef FRAUNHOFER_LC3_SUPPORT
    lc3_dec_t *lc3 = (lc3_dec_t *)pBuffer;
    if (pBuffer == NULL || !(frame_dms == 75 || frame_dms == 100)) {
        printf("%s() Invalid parameter pBuffer(%p), frame_dms(%d)\n", __FUNCTION__, pBuffer, frame_dms);
        return LC3_SET_PARAM_FAIL;
    }

    lc3->sampleRate = sampleRate;
    lc3->frame_dms  = frame_dms;
    lc3->bitrate    = bitrate;
    lc3->nChannels  = nChannels;
    lc3->bipsIn     = bitDepth;
    lc3->plcMeth    = plcMeth;
#else
    UNUSED(pBuffer);
    UNUSED(sampleRate);
    UNUSED(frame_dms);
    UNUSED(bitrate);
    UNUSED(nChannels);
    UNUSED(bitDepth);
    UNUSED(plcMeth);
    printf("%s() dummy function, not implemented\n", __FUNCTION__);
#endif
    return 0;
}

LE_SECTION_FUNC void *LC3_ENC_Init(Word8 *pBuffer)
{
#ifdef FRAUNHOFER_LC3_SUPPORT
    /* Setup Encoder */
    LC3_ErrorCode err= LC3_OK;
    int32_t encoder_size = 0;
    lc3_enc_t *lc3 = (lc3_enc_t *)pBuffer;
    err = lc3_enc_get_size(lc3->sampleRate, lc3->nChannels, lc3->bipsIn, &encoder_size);
    lc3->encoder = (LC3_Enc *)MTK_pvPortMalloc(encoder_size, LE_MALLOC_TYPE);
    err = lc3_enc_init(lc3->encoder,
                       lc3->sampleRate,
                       lc3->nChannels,
                       lc3->frame_dms,
                       0/*hrmode*/,
                       lc3->bipsIn,
                       0/*arg.lfe*/,
                       &lc3->scratch_size);

    /* set bitrate */
    lc3_enc_set_bitrate(lc3->encoder, lc3->bitrate);

    /* bitstream/pcm buffer size */
    err = lc3_enc_get_input_samples(lc3->encoder, &lc3->nSamples);
    err = lc3_enc_get_num_bytes(lc3->encoder, &lc3->nBytes);

    /* Scratch buffer */
    lc3->scratch = (void*)MTK_pvPortMalloc(lc3->scratch_size, LE_MALLOC_TYPE);
    return (void *)lc3;
#else
    UNUSED(pBuffer);
    printf("%s() dummy function, not implemented\n", __FUNCTION__);
    return NULL;
#endif
}

LE_SECTION_FUNC void *LC3_DEC_Init(Word8 *pBuffer)
{
#ifdef FRAUNHOFER_LC3_SUPPORT
    lc3_dec_t *lc3 = (lc3_dec_t *)pBuffer;
    int  decoder_size = 0;
    LC3_ErrorCode err = LC3_OK;
    float f_scal = (44100==lc3->sampleRate)? 48000.0/44100.0 : 1.0;

    /* Setup Decoder */
    err = lc3_dec_get_size(lc3->sampleRate, lc3->nChannels, LC3_PLCMETH_STD, lc3->frame_dms, 0/*arg.hrmode*/, &decoder_size);
    lc3->buffer = (LC3_Dec *)MTK_pvPortMalloc(decoder_size + 8, LE_MALLOC_TYPE);
    lc3->decoder = scratchAlign(lc3->buffer, 0);

    err = lc3_dec_init(lc3->decoder,
                       lc3->sampleRate,
                       lc3->nChannels,
                       LC3_PLCMETH_STD,
                       lc3->frame_dms,
                       0/*arg.hrmode*/,
                       0/*arg.epmode*/,
                       lc3->bipsIn,
                       &lc3->scratch_size);

    /* query bitstream/pcm size */
    err = lc3_dec_get_output_samples(lc3->decoder, &lc3->nSamples);
    lc3->nBytes = (float)(lc3->frame_dms * lc3->bitrate) * f_scal / 8000.0 / 10.0;

    lc3->scratch = MTK_pvPortMalloc(lc3->scratch_size, LE_MALLOC_TYPE);
    return (void *)lc3;
#else
    UNUSED(pBuffer);
    printf("%s() dummy function, not implemented\n", __FUNCTION__);
    return NULL;
#endif
}

LE_SECTION_FUNC void LC3_Dec_Get_Param(Word8 *pBuffer,
                                       int *decoder_size,
                                       int *scratch_size,
                                       int16_t *delay)
{
#ifdef FRAUNHOFER_LC3_SUPPORT
    LC3_ErrorCode err = LC3_OK;
    lc3_dec_t *lc3 = (lc3_dec_t *)pBuffer;
    err = lc3_dec_get_size(lc3->sampleRate, lc3->nChannels, (LC3_PlcMethod)lc3->plcMeth, lc3->frame_dms, 0, decoder_size);
    *scratch_size = lc3->scratch_size;
    err = lc3_dec_get_delay(lc3->decoder, delay);
#else
    UNUSED(pBuffer);
    UNUSED(decoder_size);
    UNUSED(scratch_size);
    UNUSED(delay);
    printf("%s() dummy function, not implemented\n", __FUNCTION__);
#endif
};

LE_SECTION_FUNC void LC3_Enc_Get_Param(Word8 *pBuffer,
                                       int *encoder_size,
                                       int *scratch_size)
{
#ifdef FRAUNHOFER_LC3_SUPPORT
    lc3_enc_t *lc3 = (lc3_enc_t *)pBuffer;
    lc3_enc_get_size(lc3->sampleRate, lc3->nChannels, lc3->bipsIn, encoder_size);
    *scratch_size = lc3->scratch_size;
#else
    UNUSED(pBuffer);
    UNUSED(encoder_size);
    UNUSED(scratch_size);
    printf("%s() dummy function, not implemented\n", __FUNCTION__);
#endif
};

LE_SECTION_FUNC Word32 LC3_DEC_GetBufferSize()
{
    return sizeof(lc3_dec_t);
}

LE_SECTION_FUNC Word32 LC3_ENC_GetBufferSize()
{
    return sizeof(lc3_enc_t);
}

LE_SECTION_FUNC void LC3_DEC_UnInit(Word8 *pBuffer)
{
#ifdef FRAUNHOFER_LC3_SUPPORT
    lc3_dec_t *lc3 = (lc3_dec_t *)pBuffer;
    if (pBuffer && lc3->decoder) {
        lc3_dec_exit(lc3->decoder);
        MTK_vPortFree(lc3->buffer);
        lc3->decoder = NULL;
        lc3->buffer = NULL;
    }
    if (pBuffer && lc3->scratch) {
        MTK_vPortFree(lc3->scratch);
        lc3->scratch = NULL;
    }
#else
    UNUSED(pBuffer);
#endif
}

LE_SECTION_FUNC void LC3_ENC_UnInit(Word8 *pBuffer)
{
#ifdef FRAUNHOFER_LC3_SUPPORT
    lc3_enc_t *lc3 = (lc3_enc_t *)pBuffer;
    if (pBuffer && lc3->encoder) {
        lc3_enc_exit(lc3->encoder);
        MTK_vPortFree(lc3->encoder);
        lc3->encoder = NULL;
    }
    if (pBuffer && lc3->scratch) {
        MTK_vPortFree(lc3->scratch);
        lc3->scratch = NULL;
    }
#else
    UNUSED(pBuffer);
#endif
}

