
#ifndef __TINYCONTROL_H
#define __TINYCONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include "asound.h"
#if defined(__cplusplus)
extern "C" {
#endif /* #if defined(__cplusplus) */

/**
 *@addtogroup TinyALSA
 *@{
 *
 *@addtogroup tinycontrol
 *@{
 * Control interface is designed to access primitive controls.
 * There is also an interface for notifying about control and strucure changes.
 *
 * In ALSA control feature, each sound card can have control elements.
 * The elements are managed according to below model.
 *
 * - \b Element \b set
 *  - A set of elements with the same attribute. Some element sets can be added to a sound card by drivers in kernel and userspace applications.
 * - \b Element
 *  - A control element might be a master volume control.
 * - \b Member
 *  - An element usually includes one or more member(s) to have a value.
 */

/**
 * \brief Opens an control device
 * \param psnd Returned control handle
 * \param name ASCII identifier of the underlying control handle
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_open(sound_t **psnd, const char *name);

/**
 * \brief close control handle
 * \param snd control handle
 * \return 0 on success otherwise a negative error code
 *
 * Closes the specified control handle and frees all associated
 * resources.
 */
int snd_ctl_close(sound_t *snd);

/**
 * \brief Set value for a control device
 * \param snd control handle
 * \param value element value
 * \retval 0 on success
 * \retval >1 on success when value was changed
 * \retval <0 a negative error code on failure
 *
 * for the purpose of connect input/output port and FE<-->BE, or volume control etc.
 */
int snd_ctl_write(sound_t *snd, struct msd_ctl_value *value);

/**
 * \brief Get value for a control device
 * \param snd control handle
 * \param value element value
 * \retval 0 on success
 * \retval <0 a negative error code on failure
 *
 * for the purpose of get input/output port and FE<-->BE status, or volume control etc.
 */
int snd_ctl_read(sound_t *snd, struct msd_ctl_value *value);
#if defined(__cplusplus)
} // extern "C"
#endif /* #if defined(__cplusplus) */

#endif /* #ifndef __TINYCONTROL_H */

