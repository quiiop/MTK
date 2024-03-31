#ifndef __TINYPCM_PLUGIN_H
#define __TINYPCM_PLUGIN_H

#include <stdint.h>
#include <stdbool.h>
#include "asound.h"
#if defined(__cplusplus)
extern "C" {
#endif /* #if defined(__cplusplus) */

typedef enum plug_type {
    FMT_CVT,
    SRC_CVT,
} plug_type_t;

typedef struct snd_plug {
    plug_type_t type;
    int id;
    int stream;
    int by_pass;
    struct msd_hw_params in_fmt;
    struct msd_hw_params out_fmt;
    size_t i_bytes_per_frame;
    size_t o_bytes_per_frame;
    struct snd_plug *slave;
    int(*process)(struct snd_plug *plug, char *in_buf, char *out_buf, size_t frames);
    char *out_buf;
    void *private_data;
} snd_plugin_t;

#define SET_IN_FMT (0x1)
#define SET_OUT_FMT (0x2)
int snd_plug_find_device(const char *name, int stream);
static inline void snd_plug_set_bypass(snd_plugin_t *plug, int bypass)
{
    plug->by_pass = bypass;
}
int snd_plug_set_params(snd_plugin_t *plug, struct msd_hw_params *param, int dir);
int snd_plug_new(snd_plugin_t **plug, plug_type_t type, int stream, int id);
int snd_plug_free(snd_plugin_t *plug);

#if defined(__cplusplus)
} // extern "C"
#endif /* #if defined(__cplusplus) */

#endif /* #ifndef __TINYPCM_PLUGIN_H */
