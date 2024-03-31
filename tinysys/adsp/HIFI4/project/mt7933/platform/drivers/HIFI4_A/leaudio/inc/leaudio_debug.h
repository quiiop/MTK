#ifndef __LEAUDIO_LOG_H__
#define __LEAUDIO_LOG_H__

#include <stdio.h>
#include <stdarg.h> /* va_list, va_start, va_arg, va_end */
#include <FreeRTOSConfig.h>
#include "mt_printf.h"

/*Definition for debugging */

#define LE_AUDIO_LOG_DEBUG
#define LE_AUDIO_LOG_INFO
#define LE_AUDIO_LOG_WARN
#define LE_AUDIO_LOG_ERR

#define pr_fmt(fmt) fmt

#ifdef LE_LOG_D
#undef LE_LOG_D
#endif
#ifdef LE_AUDIO_LOG_DEBUG
#define LE_LOG_D(fmt, ...) PRINTF_D("[D]" pr_fmt(fmt) "\n", ##__VA_ARGS__)
#else
#define LE_LOG_D(x...)
#endif

#ifdef LE_LOG_I
#undef LE_LOG_I
#endif
#ifdef LE_AUDIO_LOG_INFO
#define LE_LOG_I(fmt, ...) PRINTF_I("[I]" pr_fmt(fmt) "\n", ##__VA_ARGS__)
#else
#define LE_LOG_I(x...)
#endif

#ifdef LE_LOG_W
#undef LE_LOG_W
#endif
#ifdef LE_AUDIO_LOG_WARN
#define LE_LOG_W(fmt, ...) PRINTF_W("[W]" pr_fmt(fmt) "\n", ##__VA_ARGS__)
#else
#define LE_LOG_W(x...)
#endif

#ifdef LE_LOG_E
#undef LE_LOG_E
#endif
#ifdef LE_AUDIO_LOG_ERR
#define LE_LOG_E(fmt, ...) PRINTF_E("[E]" pr_fmt(fmt) "\n", ##__VA_ARGS__)
#else
#define LE_LOG_E(x...)
#endif


#ifdef LE_LOG_V
#undef LE_LOG_V
#endif
#define LE_LOG_V(x...)


#ifdef LE_LOG_VV
#undef LE_LOG_VV
#endif
#define LE_LOG_VV(x...)

#endif
