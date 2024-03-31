
/*
 * $Id$
 * Portable Audio I/O Library skeleton implementation
 * demonstrates how to use the common functions to implement support
 * for a host API
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2002 Ross Bencina, Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however,
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also
 * requested that these non-binding requests be included along with the
 * license above.
 */

/** @file
 @ingroup common_src

 @brief Skeleton implementation of support for a host API.
 */


#include <string.h> /* strlen() */


#define PA_ENABLE_DEBUG_OUTPUT


#define SP_PLAYQ_ENABLE



/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"


#include "pa_util.h"
#include "pa_allocation.h"
#include "pa_hostapi.h"
#include "pa_stream.h"
#include "pa_cpuload.h"
#include "pa_process.h"
#include "pa_debugprint.h"

#include "sound/include/tinypcm.h"
#include "sound/include/asound.h"
#include "audio_test_utils.h"


#define SW_GAIN_ENABLE

#ifdef SW_GAIN_ENABLE
#define VOLUME 4
#endif /* #ifdef SW_GAIN_ENABLE */

/* prototypes for functions declared in this file */

#ifdef __cplusplus
extern "C"
{
#endif /* #ifdef __cplusplus */

PaError PaSkeleton_Initialize(PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index);

static void Terminate(struct PaUtilHostApiRepresentation *hostApi);
static PaError IsFormatSupported(struct PaUtilHostApiRepresentation *hostApi,
                                 const PaStreamParameters *inputParameters,
                                 const PaStreamParameters *outputParameters,
                                 double sampleRate);
static PaError OpenStream(struct PaUtilHostApiRepresentation *hostApi,
                          PaStream **s,
                          const PaStreamParameters *inputParameters,
                          const PaStreamParameters *outputParameters,
                          double sampleRate,
                          unsigned long framesPerBuffer,
                          PaStreamFlags streamFlags,
                          PaStreamCallback *streamCallback,
                          void *userData);
static PaError CloseStream(PaStream *stream);
static PaError StartStream(PaStream *stream);
static PaError StopStream(PaStream *stream);
static PaError AbortStream(PaStream *stream);
static PaError IsStreamStopped(PaStream *s);
static PaError IsStreamActive(PaStream *stream);
static PaTime GetStreamTime(PaStream *stream);
static double GetStreamCpuLoad(PaStream *stream);
static PaError ReadStream(PaStream *stream, void *buffer, unsigned long frames);
static PaError WriteStream(PaStream *stream, const void *buffer, unsigned long frames);
static signed long GetStreamReadAvailable(PaStream *stream);
static signed long GetStreamWriteAvailable(PaStream *stream);


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */



/* IMPLEMENT ME: a macro like the following one should be used for reporting
 host errors */
#define PA_SKELETON_SET_LAST_HOST_ERROR( errorCode, errorText ) \
    PaUtil_SetLastHostErrorInfo( paInDevelopment, errorCode, errorText )

/**
 * host api datastructure specific to this implementation.
 */
typedef struct {
    PaUtilHostApiRepresentation inheritedHostApiRep;
    PaUtilStreamInterface       callbackStreamInterface;
    PaUtilStreamInterface       blockingStreamInterface;
    PaUtilAllocationGroup      *allocations;
    /* implementation specific data goes here */
}
PaSkeletonHostApiRepresentation;


PaError PaSkeleton_Initialize(PaUtilHostApiRepresentation **hostApi,
                              PaHostApiIndex hostApiIndex)
{
    PaError result = paNoError;
    int i, deviceCount;
    PaSkeletonHostApiRepresentation *skeletonHostApi;
    PaDeviceInfo *deviceInfoArray;

    skeletonHostApi = PaUtil_AllocateMemory(sizeof(*skeletonHostApi));
    if (!skeletonHostApi) {
        result = paInsufficientMemory;
        goto error;
    }

    skeletonHostApi->allocations = PaUtil_CreateAllocationGroup();
    if (!skeletonHostApi->allocations) {
        result = paInsufficientMemory;
        goto error;
    }

    *hostApi = &skeletonHostApi->inheritedHostApiRep;
    (*hostApi)->info.structVersion = 1;
    (*hostApi)->info.type          = paInDevelopment;            /* IMPLEMENT ME: change to correct type id */
    (*hostApi)->info.name          = "mt7933";

    (*hostApi)->info.defaultInputDevice  = 0; /* IMPLEMENT ME */
    (*hostApi)->info.defaultOutputDevice = 0; /* IMPLEMENT ME */

    (*hostApi)->info.deviceCount = 1;

    deviceCount = 1; /* IMPLEMENT ME */

    if (deviceCount > 0) {
        (*hostApi)->deviceInfos = (PaDeviceInfo **)PaUtil_GroupAllocateMemory(
                                      skeletonHostApi->allocations, sizeof(PaDeviceInfo *) * deviceCount);
        if (!(*hostApi)->deviceInfos) {
            result = paInsufficientMemory;
            goto error;
        }

        /* allocate all device info structs in a contiguous block */
        deviceInfoArray = (PaDeviceInfo *)PaUtil_GroupAllocateMemory(
                              skeletonHostApi->allocations, sizeof(PaDeviceInfo) * deviceCount);
        if (!deviceInfoArray) {
            result = paInsufficientMemory;
            goto error;
        }

        for (i = 0; i < deviceCount; ++i) {
            PaDeviceInfo *deviceInfo = &deviceInfoArray[i];
            deviceInfo->structVersion = 2;
            deviceInfo->hostApi = hostApiIndex;
            deviceInfo->name = "_mt7933"; /* IMPLEMENT ME: allocate block and copy name eg:
                deviceName = (char*)PaUtil_GroupAllocateMemory( skeletonHostApi->allocations, strlen(srcName) + 1 );
                if( !deviceName )
                {
                    result = paInsufficientMemory;
                    goto error;
                }
                strcpy( deviceName, srcName );
                deviceInfo->name = deviceName;
            */

            deviceInfo->maxInputChannels = 2;  /* IMPLEMENT ME */
            deviceInfo->maxOutputChannels = 2;  /* IMPLEMENT ME */

            deviceInfo->defaultLowInputLatency = 0.09;  /* IMPLEMENT ME */
            deviceInfo->defaultLowOutputLatency = 0.09;  /* IMPLEMENT ME */
            deviceInfo->defaultHighInputLatency = 0.18;  /* IMPLEMENT ME */
            deviceInfo->defaultHighOutputLatency = 0.18;  /* IMPLEMENT ME */

            deviceInfo->defaultSampleRate = 8000; /* IMPLEMENT ME */

            (*hostApi)->deviceInfos[i] = deviceInfo;
            ++(*hostApi)->info.deviceCount;
        }
    }

    (*hostApi)->Terminate = Terminate;
    (*hostApi)->OpenStream = OpenStream;
    (*hostApi)->IsFormatSupported = IsFormatSupported;

    PaUtil_InitializeStreamInterface(&skeletonHostApi->callbackStreamInterface, CloseStream, StartStream,
                                     StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                     GetStreamTime, GetStreamCpuLoad,
                                     PaUtil_DummyRead, PaUtil_DummyWrite,
                                     PaUtil_DummyGetReadAvailable, PaUtil_DummyGetWriteAvailable);

    PaUtil_InitializeStreamInterface(&skeletonHostApi->blockingStreamInterface, CloseStream, StartStream,
                                     StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                     GetStreamTime, PaUtil_DummyGetCpuLoad,
                                     ReadStream, WriteStream, GetStreamReadAvailable, GetStreamWriteAvailable);

    return result;

error:
    if (skeletonHostApi) {
        if (skeletonHostApi->allocations) {
            PaUtil_FreeAllAllocations(skeletonHostApi->allocations);
            PaUtil_DestroyAllocationGroup(skeletonHostApi->allocations);
        }

        PaUtil_FreeMemory(skeletonHostApi);
    }
    return result;
}


static void Terminate(struct PaUtilHostApiRepresentation *hostApi)
{
    PaSkeletonHostApiRepresentation *skeletonHostApi = (PaSkeletonHostApiRepresentation *)hostApi;

    if (skeletonHostApi->allocations) {
        PaUtil_FreeAllAllocations(skeletonHostApi->allocations);
        PaUtil_DestroyAllocationGroup(skeletonHostApi->allocations);
    }

    PaUtil_FreeMemory(skeletonHostApi);
}


static PaError IsFormatSupported(struct PaUtilHostApiRepresentation *hostApi,
                                 const PaStreamParameters *inputParameters,
                                 const PaStreamParameters *outputParameters,
                                 double sampleRate)
{
    int inputChannelCount, outputChannelCount;
    PaSampleFormat inputSampleFormat, outputSampleFormat;

    if (inputParameters) {
        inputChannelCount = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;

        /* all standard sample formats are supported by the buffer adapter,
            this implementation doesn't support any custom sample formats */
        if (inputSampleFormat & paCustomFormat)
            return paSampleFormatNotSupported;

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */

        if (inputParameters->device == paUseHostApiSpecificDeviceSpecification)
            return paInvalidDevice;

        /* check that input device can support inputChannelCount */
        if (inputChannelCount > hostApi->deviceInfos[ inputParameters->device ]->maxInputChannels)
            return paInvalidChannelCount;

        /* validate inputStreamInfo */
        if (inputParameters->hostApiSpecificStreamInfo)
            return paIncompatibleHostApiSpecificStreamInfo; /* this implementation doesn't use custom stream info */
    } else {
        inputChannelCount = 0;
    }

    if (outputParameters) {
        outputChannelCount = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;

        /* all standard sample formats are supported by the buffer adapter,
            this implementation doesn't support any custom sample formats */
        if (outputSampleFormat & paCustomFormat)
            return paSampleFormatNotSupported;

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */

        if (outputParameters->device == paUseHostApiSpecificDeviceSpecification)
            return paInvalidDevice;

        /* check that output device can support outputChannelCount */
        if (outputChannelCount > hostApi->deviceInfos[ outputParameters->device ]->maxOutputChannels)
            return paInvalidChannelCount;

        /* validate outputStreamInfo */
        if (outputParameters->hostApiSpecificStreamInfo)
            return paIncompatibleHostApiSpecificStreamInfo; /* this implementation doesn't use custom stream info */
    } else {
        outputChannelCount = 0;
    }

    /* suppress unused variable warnings */
    (void) sampleRate;

    return paFormatIsSupported;
}

/* PaSkeletonStream - a stream data structure specifically for this implementation */

typedef enum {
    IN,
    OUT,
    INVALID
} Direction;


typedef struct PaSkeletonStream {
    /* IMPLEMENT ME: rename this */
    PaUtilStreamRepresentation streamRepresentation;
    PaUtilCpuLoadMeasurer cpuLoadMeasurer;
    PaUtilBufferProcessor bufferProcessor;

    /* IMPLEMENT ME:
            - implementation specific data goes here
    */

    sound_t *w_snd;
    sound_t *r_snd;

    struct msd_hw_params w_params;
    struct msd_hw_params r_params;
    int bitdepth;
    int bytes_per_frame;
    int isActive;
    Direction direction;
    unsigned long framesPerHostCallback; /* just an example */
}
PaSkeletonStream;


#ifdef SP_PLAYQ_ENABLE

enum {
    PQ_CMD_START = 0,
    PQ_CMD_STOP,
};

enum {
    PQ_ST_IDLE = 0,
    PQ_ST_BUFFER,
    PQ_ST_START,
    PQ_ST_STOP,
};

typedef struct S_PLAYQ_DATA {
    char   *ucBuf;
    size_t  size;
    void   *userData;
} T_PLAYQ_DATA;

#define T_PLAYQ_CMD    T_PLAYQ_DATA

#define QUEUE_NO_WAIT                        ( (TickType_t ) 0 )
#define QUEUE_WAIT_THRESHOLD                 ( (TickType_t ) 15 )
#define SPQ_PRE_BUFFER_NUM                   128 //Unit: 960 frames, 960*128=122880(frames), 20ms*128=2560ms
#undef  SPQ_PRE_BUFFER_NUM
#define SPQ_PRE_BUFFER_NUM                   25 //Unit: 960 frames, 960*128=122880(frames), 20ms*25=500ms
#define SPQ_DATA_TIMEOUT                     30  // Unit:20ms,  20ms*30 = 600ms
#define SPQ_DATA_STOP                        100 // Unit:20ms,  20ms*100 = 2s
#define MAX_SPQ_RETRY                        5
#define MAX_SPQ_CMD_Q_NUM                    10
#define MAX_SPQ_DATA_Q_NUM                   3840

QueueHandle_t  xSPQCmdQ   = NULL;
QueueHandle_t  xSPQDataQ  = NULL;
TaskHandle_t   xSPQHandle = NULL;
sound_t       *g_w_snd    = NULL;
struct msd_hw_params g_w_params;

uint8_t aia_port_volume = 0;

#include "filogic_led.h"

void sp_playq_task(void *pvParameters);

extern int _ram_tfm_start[];
extern int _ram_tfm_length[];
/** Releases memory allocated by a call to @c AiaCalloc(). */
static inline void AiaPortFree(void *ptr)
{
    if ((ptr > (void *)_ram_tfm_start) && (ptr < (void *)((uint32_t)_ram_tfm_start + (uint32_t)_ram_tfm_length))) {
        vPortFreeExt(aiaRegionID, ptr);
    } else {
        vPortFree(ptr);
    }
}


static inline void *AiaPortMalloc(size_t size)
{
    void *ptr = pvPortMallocExt(aiaRegionID, size);

    if (!ptr) {
        ptr = pvPortMalloc(size);
    }
    return ptr;
}



int sp_playq_init(void)
{
    // Skip Re-Create if All Task/Queue are ready
    if (xSPQCmdQ && xSPQDataQ && xSPQHandle) {
        return 0;
    }

    // Create - Speaker Play Command Queue
    if (! xSPQCmdQ) {
        xSPQCmdQ = xQueueCreate(MAX_SPQ_CMD_Q_NUM, sizeof(T_PLAYQ_CMD));
    }

    // Create - Speaker Play Data Queue
    if (! xSPQDataQ) {
        xSPQDataQ = xQueueCreate(MAX_SPQ_DATA_Q_NUM, sizeof(T_PLAYQ_DATA));
    }

    // Create - Speaker Play Task
    if (!xSPQHandle) {
        xTaskCreate(sp_playq_task, "spq", 1024 * 2, NULL,
                    (tskIDLE_PRIORITY + 18), &xSPQHandle);
    }

    printf("[playq]: Speaker Play - Task(%p), CmdQ(%p), DataQ(%p)\n",
           xSPQHandle, xSPQCmdQ, xSPQDataQ);

    if (! xSPQDataQ || ! xSPQHandle || ! xSPQCmdQ) {
        printf("[playq]: Controll Task Create Fail\n");
        vTaskDelete(xSPQHandle);
        vQueueDelete(xSPQCmdQ);
        vQueueDelete(xSPQDataQ);

        xSPQCmdQ  = NULL;
        xSPQDataQ = NULL;
        xSPQHandle = NULL;

        return -1;
    }

    return 0;
}

void sp_playq_task(void *pvParameters)
{
    int               status       = PQ_ST_IDLE;  // 0:Idle, 1:Start, 2:Try to Stop
    int               retry        = 0;
    int16_t          *silence      = NULL;
    int               silence_size = 960 / 4 * 3;
    int               silence_send = 0;
    TickType_t        dataWait    = QUEUE_NO_WAIT;
    TickType_t        cmdWait      = portMAX_DELAY;
    T_PLAYQ_CMD       cmd;
    T_PLAYQ_DATA      data;

    int snd_status = 0;
    int led_status = 0;

    silence = (int16_t *)PaUtil_AllocateMemory(silence_size * sizeof(int16_t));
    memset(silence, 0, silence_size * sizeof(int16_t));

    int16_t *volume_adjusted_buf = NULL;

    while (1) {
        // Wait & Receive PlayQ Cmd - Start/Stop
        if (xQueueReceive(xSPQCmdQ, &cmd, cmdWait) == pdPASS) {
            switch ((int) cmd.ucBuf) {
                case PQ_CMD_START : { // Start to Play
                        printf("[playq] cmd start %u\n",
                               (unsigned int)xTaskGetTickCount());
                        status   = PQ_ST_BUFFER;
                        cmdWait  = 20 * portTICK_PERIOD_MS;
                        break;
                    }
                case PQ_CMD_STOP : { // Stop to Play
                        status   = PQ_ST_STOP;
                        cmdWait  = portMAX_DELAY;
                        dataWait = QUEUE_NO_WAIT;
                        break;
                    }
                default: {
                        printf("[playq]: Unknown Queue Cmd!\n");
                        break;
                    }
            }
        }

        switch (status) {
            case PQ_ST_BUFFER: {
                    if (uxQueueMessagesWaiting(xSPQDataQ) >= SPQ_PRE_BUFFER_NUM) {
                        status   = PQ_ST_START;
                        cmdWait  = QUEUE_NO_WAIT;
                        dataWait = QUEUE_WAIT_THRESHOLD;
                        printf("[playq] pcm start %u\n",
                               (unsigned int)xTaskGetTickCount());
                        if (led_status == 0) {
                            led_status = 1;
                            filogic_led_light_toggle(1);
                        }

                    }
                    break;
                }
            case PQ_ST_START: {
                    if (xQueueReceive(xSPQDataQ, &data, dataWait) == pdPASS) {
                        /* Open snd if it is closed */
                        if (snd_status == 0) {
                            snd_status = 1;
                            connect_route("track0", "INTDAC_out", 1, CONNECT_FE_BE);
                            connect_route("I_22", "O_20", 1, CONNECT_IO_PORT);
                            connect_route("I_23", "O_21", 1, CONNECT_IO_PORT);
                            snd_pcm_open(&g_w_snd, "track0", 0, 0); // blocking mode
                            snd_pcm_hw_params(g_w_snd, &g_w_params);
                            snd_pcm_prepare(g_w_snd);
                        }
                        if (led_status == 0) {
                            led_status = 1;
                            filogic_led_light_toggle(1);
                        }
                        if (g_w_snd) {
                            /* Adjust volume in spq task instead of AIA speaker manager */
                            if (aia_port_volume != 0) {
                                volume_adjusted_buf = (int16_t *)data.ucBuf;
                                for (size_t i = 0; i < data.size; ++i) {
                                    volume_adjusted_buf[i] = volume_adjusted_buf[ i ] * aia_port_volume / 100;
                                }
                            }
                            snd_pcm_write(g_w_snd, (int16_t *)data.ucBuf,
                                          data.size);
                        }
                        if (data.ucBuf) {
                            AiaPortFree(data.ucBuf);
                        }
                        retry = 0;
                        silence_send = 0;
                    } else {
                        if (g_w_snd) {
                            /* Close snd instead of writing silence */
                            if (snd_status == 1) {
                                snd_status = 0;
                                snd_pcm_drain(g_w_snd);
                                snd_pcm_hw_free(g_w_snd);
                                snd_pcm_close(g_w_snd);
                                connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
                                connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
                                connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);
                            }
                            if (led_status == 1) {
                                led_status = 0;
                                filogic_led_light_toggle(0);
                            }
                        }

                        silence_send++;
                        if (silence_send > 500) {
                            cmdWait     = 20 * portTICK_PERIOD_MS;
                            status      = PQ_ST_BUFFER;
                            silence_send = 0;
                        }
                    }
                    break;
                }
            case PQ_ST_STOP: {
                    while (xQueueReceive(xSPQDataQ, &data, QUEUE_NO_WAIT) == pdPASS) {
                        if (data.ucBuf) {
                            AiaPortFree(data.ucBuf);
                        }
                    }

                    if (g_w_snd) {
                        /* Close snd instead of writing silence */
                        if (snd_status == 1) {
                            snd_status = 0;
                            snd_pcm_drain(g_w_snd);
                            snd_pcm_hw_free(g_w_snd);
                            snd_pcm_close(g_w_snd);
                            connect_route("track0", "INTDAC_out", 0, CONNECT_FE_BE);
                            connect_route("I_22", "O_20", 0, CONNECT_IO_PORT);
                            connect_route("I_23", "O_21", 0, CONNECT_IO_PORT);
                        }
                        if (led_status == 1) {
                            led_status = 0;
                            filogic_led_light_toggle(0);
                        }
                    }

                    status   = PQ_ST_IDLE;
                    dataWait = QUEUE_NO_WAIT;
                    cmdWait  = portMAX_DELAY;
                    break;
                }
            default:
                break;
        }
    }

    PaUtil_FreeMemory(silence);
    vQueueDelete(xSPQDataQ);
    vQueueDelete(xSPQCmdQ);
    xSPQDataQ = NULL;
    xSPQCmdQ  = NULL;
    vTaskDelete(NULL);

    return;
}

void sp_playq_start(void)
{
    int            resend = 0;
    T_PLAYQ_CMD    cmd;

    //Send Start Play Cmd
    cmd.ucBuf    = (char *)PQ_CMD_START;
    cmd.size     = 0;
    cmd.userData = (void *)NULL;

    while ((resend++ < 3) && (xQueueSend(xSPQCmdQ, (void *)&cmd, (TickType_t) QUEUE_NO_WAIT) != pdPASS)) {
        printf("[playq]: Send CmdQ Fail!\n");
    }

    printf("[playq] Start Cmd Send ...\n");

    return;
}


void sp_playq_write(PaSkeletonStream *stream, void *buffer, unsigned long frames)
{
    T_PLAYQ_DATA   data;
    void           *buf;
    buf = AiaPortMalloc(sizeof(int16_t) * frames);
    if (buf == NULL) {
        printf("[playq] sp_playq_write pvPortMalloc failed!\n");
        return;
    }

    memcpy(buf, buffer, sizeof(int16_t) * frames);

    data.ucBuf    = (char *)buf;
    data.size     = frames;
    data.userData = (void *)g_w_snd;

    // Send Data to Speaker Play Queue
    if (xQueueSend(xSPQDataQ, (void *)&data, QUEUE_NO_WAIT) != pdPASS) {
        printf("[playq] Speaker Play DataQ Full!\n");
        AiaPortFree(buf);
        return;
    }

    return;
}

void sp_playq_stop(PaSkeletonStream *stream)
{
    int            resend = 0;
    T_PLAYQ_CMD    cmd;

    //Send Start Play Cmd
    cmd.ucBuf    = (char *)PQ_CMD_STOP;
    cmd.size     = 0;
    cmd.userData = (void *)g_w_snd;

    while ((resend++ < 3) && (xQueueSend(xSPQCmdQ, (void *)&cmd, (TickType_t) QUEUE_NO_WAIT) != pdPASS)) {
        printf("[playq] Send CmdQ Fail!\n");
    }

    printf("[playq] SPlayQ Stop Cmd Send ...\n");

    return;
}

/* for AIA sample app to control speaker status */
void aia_port_playback_stop(void)
{
    sp_playq_stop(NULL);
}

void aia_port_playback_start(void)
{
    sp_playq_start();
}

#endif /* #ifdef SP_PLAYQ_ENABLE */


static PaError OpenStream(struct PaUtilHostApiRepresentation *hostApi,
                          PaStream **s,
                          const PaStreamParameters *inputParameters,
                          const PaStreamParameters *outputParameters,
                          double sampleRate,
                          unsigned long framesPerBuffer,
                          PaStreamFlags streamFlags,
                          PaStreamCallback *streamCallback,
                          void *userData)
{
    PaError result = paNoError;
    PaSkeletonHostApiRepresentation *skeletonHostApi = (PaSkeletonHostApiRepresentation *)hostApi;
    PaSkeletonStream *stream = 0;
    unsigned long framesPerHostBuffer = framesPerBuffer; /* these may not be equivalent for all implementations */
    int inputChannelCount, outputChannelCount;
    PaSampleFormat inputSampleFormat, outputSampleFormat;
    PaSampleFormat hostInputSampleFormat, hostOutputSampleFormat;

    if (inputParameters) {
        inputChannelCount = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */

        if (inputParameters->device == paUseHostApiSpecificDeviceSpecification)
            return paInvalidDevice;

        /* check that input device can support inputChannelCount */
        if (inputChannelCount > hostApi->deviceInfos[ inputParameters->device ]->maxInputChannels)
            return paInvalidChannelCount;

        /* validate inputStreamInfo */
        if (inputParameters->hostApiSpecificStreamInfo)
            return paIncompatibleHostApiSpecificStreamInfo; /* this implementation doesn't use custom stream info */

        /* IMPLEMENT ME - establish which  host formats are available */
        hostInputSampleFormat =
            PaUtil_SelectClosestAvailableFormat(paInt16 /* native formats */, inputSampleFormat);
    } else {
        inputChannelCount = 0;
        inputSampleFormat = hostInputSampleFormat = paInt16; /* Surpress 'uninitialised var' warnings. */
    }

    if (outputParameters) {
        outputChannelCount = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */

        if (outputParameters->device == paUseHostApiSpecificDeviceSpecification)
            return paInvalidDevice;

        /* check that output device can support inputChannelCount */
        if (outputChannelCount > hostApi->deviceInfos[ outputParameters->device ]->maxOutputChannels)
            return paInvalidChannelCount;

        /* validate outputStreamInfo */
        if (outputParameters->hostApiSpecificStreamInfo)
            return paIncompatibleHostApiSpecificStreamInfo; /* this implementation doesn't use custom stream info */

        /* IMPLEMENT ME - establish which host formats are available */
        hostOutputSampleFormat =
            PaUtil_SelectClosestAvailableFormat(paInt16 /* native formats */, outputSampleFormat);
    } else {
        outputChannelCount = 0;
        outputSampleFormat = hostOutputSampleFormat = paInt16; /* Surpress 'uninitialized var' warnings. */
    }

    /* validate platform specific flags */
    if ((streamFlags & paPlatformSpecificFlags) != 0)
        return paInvalidFlag; /* unexpected platform specific flag */


    stream = (PaSkeletonStream *)PaUtil_AllocateMemory(sizeof(PaSkeletonStream));

    /*hw parameter setting*/
    int ret = 0, index;
    stream->bitdepth        = 16;
    stream->bytes_per_frame = stream->bitdepth * 1 / 8;
    stream->isActive        = 0;
    if (outputChannelCount) {
#ifdef SP_PLAYQ_ENABLE
        sp_playq_init();
#endif /* #ifdef SP_PLAYQ_ENABLE */
        stream->direction             = OUT;
        stream->w_params.format       = MSD_PCM_FMT_S16_LE;
        stream->w_params.channels     = 1;
        stream->w_params.period_count = 4;
        stream->w_params.period_size  = 3840;
        stream->w_params.rate         = 48000;
#ifdef SP_PLAYQ_ENABLE
        g_w_snd = stream->w_snd;
        g_w_params = stream->w_params;
#endif /* #ifdef SP_PLAYQ_ENABLE */
    }

    if (inputChannelCount) {
        stream->direction             = IN;
#ifdef AIA_AP_RECORD
        stream->r_params.format       = MSD_PCM_FMT_S16_LE;
        stream->r_params.channels     = 1;
        stream->r_params.period_count = 12;
        stream->r_params.period_size  = 1280;
        stream->r_params.rate         = 16000;
        connect_route("UL9", "INTADC_in", 1, CONNECT_FE_BE);
        connect_route("I_60", "O_26", 1, CONNECT_IO_PORT);
        connect_route("I_61", "O_27", 1, CONNECT_IO_PORT);
        connect_route("I_08", "O_28", 1, CONNECT_IO_PORT);
        connect_route("I_09", "O_29", 1, CONNECT_IO_PORT);

        ret = snd_pcm_open(&stream->r_snd, "UL9", 0, 0);   // blocking mode
        if (ret)
            goto exit1;
        ret = snd_pcm_hw_params(stream->r_snd, &stream->r_params);
        if (ret)
            goto exit2;
        ret = snd_pcm_prepare(stream->r_snd);
        if (ret)
            goto exit2;
#endif /* #ifdef AIA_AP_RECORD */
    }


    if (!stream) {
        result = paInsufficientMemory;
        goto error;
    }

    if (streamCallback) {
        PaUtil_InitializeStreamRepresentation(&stream->streamRepresentation,
                                              &skeletonHostApi->callbackStreamInterface,
                                              streamCallback, userData);
    } else {
        PaUtil_InitializeStreamRepresentation(&stream->streamRepresentation,
                                              &skeletonHostApi->blockingStreamInterface,
                                              streamCallback, userData);
    }

    PaUtil_InitializeCpuLoadMeasurer(&stream->cpuLoadMeasurer, sampleRate);

    stream->framesPerHostCallback = framesPerHostBuffer;
    *s = (PaStream *)stream;

    return result;

error:
    if (stream)
        PaUtil_FreeMemory(stream);


exit2:
#ifdef AIA_AP_RECORD
    ret = snd_pcm_close(stream->r_snd);
#endif /* #ifdef AIA_AP_RECORD */
    if (ret)
        goto exit0;

exit0:
    if (inputChannelCount) {
        connect_route("UL9", "INTADC_in", 0, CONNECT_FE_BE);
        connect_route("I_60", "O_26", 0, CONNECT_IO_PORT);
        connect_route("I_61", "O_27", 0, CONNECT_IO_PORT);
        connect_route("I_08", "O_28", 0, CONNECT_IO_PORT);
        connect_route("I_09", "O_29", 0, CONNECT_IO_PORT);
    }

    if (outputChannelCount) {
#ifdef SP_PLAYQ_ENABLE
        g_w_snd = NULL;
#endif /* #ifdef SP_PLAYQ_ENABLE */
    }

    return result;
}


static PaError CloseStream(PaStream *s)
{
    PaError           result = paNoError;
    PaSkeletonStream *stream = (PaSkeletonStream *)s;
    //int avail;

    if (stream->direction == OUT) {
#ifdef SP_PLAYQ_ENABLE
        g_w_snd = NULL;
#endif /* #ifdef SP_PLAYQ_ENABLE */
    }

    if (stream->direction == IN) {
#ifdef AIA_AP_RECORD
        snd_pcm_close(stream->r_snd);
        connect_route("UL9", "INTADC_in", 0, CONNECT_FE_BE);
        connect_route("I_60", "O_26", 0, CONNECT_IO_PORT);
        connect_route("I_61", "O_27", 0, CONNECT_IO_PORT);
        connect_route("I_08", "O_28", 0, CONNECT_IO_PORT);
        connect_route("I_09", "O_29", 0, CONNECT_IO_PORT);
        snd_pcm_hw_free(stream->r_snd);
#endif /* #ifdef AIA_AP_RECORD */
    }

    PaUtil_TerminateStreamRepresentation(&stream->streamRepresentation);
    PaUtil_FreeMemory(stream);

    return result;
}

extern sound_t *upload_snd;

static PaError StartStream(PaStream *s)
{
    PaError           result = paNoError;
    PaSkeletonStream *stream = (PaSkeletonStream *)s;

#ifdef AIA_AP_RECORD
    if (stream->direction == IN)
        snd_pcm_start(stream->r_snd);
#endif /* #ifdef AIA_AP_RECORD */

    if (stream->direction == OUT) {
#ifdef SP_PLAYQ_ENABLE
        //Send Start Play Cmd
        sp_playq_start();
#endif /* #ifdef SP_PLAYQ_ENABLE */
    }
    stream->isActive = 1;

    return result;
}

extern volatile int aia_va_record_flag;
static PaError StopStream(PaStream *s)
{
    PaError           result = paNoError;
    PaSkeletonStream *stream = (PaSkeletonStream *)s;
    //int avail;

    if (stream->direction == OUT) {
#ifdef SP_PLAYQ_ENABLE
        //Send Start Play Cmd
        sp_playq_stop(stream);
#endif /* #ifdef SP_PLAYQ_ENABLE */
    }

    if (stream->direction == IN) {
#ifdef AIA_AP_RECORD
        snd_pcm_drop(stream->r_snd);
#else /* #ifdef AIA_AP_RECORD */
        aia_va_record_flag = 0;
#endif /* #ifdef AIA_AP_RECORD */
    }

    stream->isActive = 0;
    return result;
}


static PaError AbortStream(PaStream *s)
{
    PaError           result = paNoError;
    PaSkeletonStream *stream = (PaSkeletonStream *)s;
    //int avail;

    /* IMPLEMENT ME, see portaudio.h for required behavior */
    if (stream->direction == OUT) {
#ifdef SP_PLAYQ_ENABLE
        //Send Start Play Cmd
        sp_playq_stop(stream);
#endif /* #ifdef SP_PLAYQ_ENABLE */
    }

#ifdef AIA_AP_RECORD
    if (stream->direction == IN)
        snd_pcm_drop(stream->r_snd);
#endif /* #ifdef AIA_AP_RECORD */
    return result;
}


static PaError IsStreamStopped(PaStream *s)
{
    PaSkeletonStream *stream = (PaSkeletonStream *)s;
    return ! stream->isActive;
}


static PaError IsStreamActive(PaStream *s)
{
    PaSkeletonStream *stream = (PaSkeletonStream *)s;
    return stream->isActive;
}


static PaTime GetStreamTime(PaStream *s)
{
    PaSkeletonStream *stream = (PaSkeletonStream *)s;
    return 0;
}


static double GetStreamCpuLoad(PaStream *s)
{
    PaSkeletonStream *stream = (PaSkeletonStream *)s;
    return PaUtil_GetCpuLoad(&stream->cpuLoadMeasurer);
}

#ifndef AIA_AP_RECORD
#include "stream_buffer.h"
extern StreamBufferHandle_t xAiaRecordingStreamBuffer;
#endif /* #ifndef AIA_AP_RECORD */

static PaError ReadStream(PaStream *s,
                          void *buffer,
                          unsigned long frames)
{
    PaSkeletonStream *stream   = (PaSkeletonStream *)s;
#ifdef AIA_AP_RECORD
    int               ret      = 0;
    int               bytesGot = 0;

    while (frames > 0) {
        //ret = snd_pcm_read( stream->r_snd, buffer + bytesGot, stream->r_params.period_size);
        ret       = snd_pcm_read(stream->r_snd, buffer + bytesGot, frames);
        if (ret < 0) {
            return paInternalError;
        }
        frames   -= ret;
        bytesGot += ret * stream->bytes_per_frame;
    }

#ifdef SW_GAIN_ENABLE
    int u_bnd     = 32767, l_bnd = -32768;
    int i         = 0;
    short *sample = (short *)buffer;
    int data_size = frames * stream->bytes_per_frame;

    for (; i < data_size / 2; i++) {
        if (sample[i] > u_bnd / VOLUME)
            sample[i] = (short)u_bnd;
        else if (sample[i] < l_bnd / VOLUME)
            sample[i] = (short)l_bnd;
        else
            sample[i] = sample[ i ] * VOLUME;
    }
#endif /* #ifdef SW_GAIN_ENABLE */

#else /* #ifdef AIA_AP_RECORD */

    size_t xReceivedBytes;
    xReceivedBytes = xStreamBufferReceive(xAiaRecordingStreamBuffer,
                                          (void *) buffer,
                                          frames * 2,
                                          0);
#endif /* #ifdef AIA_AP_RECORD */
    return paNoError;
}


static PaError WriteStream(PaStream *s,
                           const void *buffer,
                           unsigned long frames)
{
    PaSkeletonStream *stream = (PaSkeletonStream *)s;
    long    sframes  = (long)frames;
    int     ret      = 0;
    int     bytesGot = 0;
    extern int xrun;

#ifdef SP_PLAYQ_ENABLE
    sp_playq_write(stream, buffer, frames);
#endif /* #ifdef SP_PLAYQ_ENABLE */

    return paNoError;
}


static signed long GetStreamReadAvailable(PaStream *s)
{
    PaSkeletonStream *stream = (PaSkeletonStream *)s;

    /* suppress unused variable warnings */
    (void) stream;
#ifdef AIA_AP_RECORD
    int avail;
    avail = snd_pcm_avail(stream->r_snd);

    return (signed long)avail;
#else /* #ifdef AIA_AP_RECORD */
    size_t avail = xStreamBufferBytesAvailable(xAiaRecordingStreamBuffer);
    return (avail / 2);
#endif /* #ifdef AIA_AP_RECORD */
}


static signed long GetStreamWriteAvailable(PaStream *s)
{
    PaSkeletonStream *stream = (PaSkeletonStream *)s;

    /* suppress unused variable warnings */
    (void) stream;
    int avail;

#ifdef SP_PLAYQ_ENABLE
    avail = 3840 * 4;
#endif /* #ifdef SP_PLAYQ_ENABLE */

    return (signed long)avail;
}

