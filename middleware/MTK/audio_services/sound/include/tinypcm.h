#ifndef __TINYPCM_H
#define __TINYPCM_H

#include <stdint.h>
#include <stdbool.h>
#include "asound.h"
#ifdef MAS_AGENT_SUPPORT
#include "tinypcm_plugin.h"

#define PLUGIN_FIXED_FORMAT MSD_PCM_FMT_S32_LE
#endif /* #ifdef MAS_AGENT_SUPPORT */

#define SND_PCM_CORE_VERSION    "1.0.0"
#define SND_PCM_DRIVER_VERSION  "0.5.0"

#define snd_pcm_write snd_pcm_writei
#define snd_pcm_read snd_pcm_readi
#if defined(__cplusplus)
extern "C" {
#endif /* #if defined(__cplusplus) */
/**
 *@addtogroup TinyALSA
 *@{
 * @brief This section introduces the TinyALSA APIs including terms and acronyms,
 * supported features, details on how to use the MiniCLI, function groups, enums,
 * structures and functions.
 *
 * TinyALSA is a small library to interface with ALSA in the linux kernel.
 *
 * The aims are:
 * - Provide a basic pcm and mixer API.
 * - If it's not absolutely needed, don't add it the API.
 * - Avoid supporting complex and unnecessary operations, that could be dealt with at a higher level.
 *
 *@addtogroup tinypcm
 *@{
 * ALSA uses the ring buffer to store outgoing (playback) and incoming (capture, record) samples.
 * There are two pointers being maintained to allow a precise communication between application
 * and device pointing to current processed sample by hardware and last processed sample by application.
 * The modern audio chips allow to program the transfer time periods.
 * It means that the stream of samples is divided to small chunk.
 * Device acknowledges to application when the transfer of a chunk is complete.
 */

/**
 * \brief Opens a PCM
 * \param psnd Returned PCM handle
 * \param name ASCII identifier of the PCM handle
 * \param stream Wanted stream
 * \param mode Open mode (see #SND_PCM_NONBLOCK, #SND_PCM_ASYNC)
 * \return 0 on success otherwise a negative error code
 */
int snd_pcm_open(sound_t **psnd, const char *name, int stream, int mode);

/**
 * \brief Write interleaved frames to a PCM
 * \param snd PCM handle
 * \param buf frames containing buffer
 * \param size frames to be written
 * \return a positive number of frames actually written otherwise a
 * negative error code
 */
int snd_pcm_writei(sound_t *snd, void *buf, unsigned int size);

/**
 * \brief Read interleaved frames from a PCM
 * \param snd PCM handle
 * \param buf frames containing buffer
 * \param size frames to be read
 * \return a positive number of frames actually read otherwise a
 * negative error code
 */
int snd_pcm_readi(sound_t *snd, void *buf, unsigned int size);


/** \brief Install one PCM hardware configuration
 * \param snd PCM handle
 * \param params Configuration space definition container
 * \return 0 on success otherwise a negative error code
 */
int snd_pcm_hw_params(sound_t *snd, struct msd_hw_params *params);

/** \brief Install PCM software configuration defined by params
 * \param snd PCM handle
 * \param params Configuration container
 * \return 0 on success otherwise a negative error code
 */
int snd_pcm_sw_params(sound_t *snd, struct msd_sw_params *params);

/**
 * \brief Start a PCM
 * \param snd PCM handle
 * \return 0 on success otherwise a negative error code
 */
int snd_pcm_start(sound_t *snd);

/**
 * \brief Prepare PCM for use
 * \param snd PCM handle
 * \return 0 on success otherwise a negative error code
 *
 */
int snd_pcm_prepare(sound_t *snd);


/** \brief Remove PCM hardware configuration and free associated resources
 * \param snd PCM handle
 * \return 0 on success otherwise a negative error code
 */
int snd_pcm_hw_free(sound_t *snd);

/**
 * \brief Stop a PCM dropping pending frames
 * \param snd PCM handle
 * \return 0 on success otherwise a negative error code
 */
int snd_pcm_drop(sound_t *snd);

/**
 * \brief Stop a PCM preserving pending frames
 * \param snd PCM handle
 * \return 0 on success otherwise a negative error code
 *
 * For playback wait for all pending frames to be played and then stop
 * the PCM.
 * For capture stop PCM permitting to retrieve residual frames.
 *
 */
int snd_pcm_drain(sound_t *snd);

/**
 * \brief close PCM handle
 * \param snd PCM handle
 * \return 0 on success otherwise a negative error code
 *
 * Closes the specified PCM handle and frees all associated
 * resources.
 */
int snd_pcm_close(sound_t *snd);

/**
*
* Get the available(writable) space for playback
* Get the available(readable) space for capture
*/
int snd_pcm_avail(sound_t *snd);

#ifdef MAS_AGENT_SUPPORT
typedef int (*mas_stream_cb_t)(int event, void *event_data, void *usr);
int snd_pcm_set_cb(sound_t *snd, mas_stream_cb_t cb, void *usr);
int snd_pcm_set_master_vol(int level);
#endif /* #ifdef MAS_AGENT_SUPPORT */

#if defined(__cplusplus)
} // extern "C"
#endif /* #if defined(__cplusplus) */

#endif /* #ifndef __TINYPCM_H */

