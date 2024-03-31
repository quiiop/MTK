// -*- C++ -*-
//===--------------------- support/xtensa/xlocale.h -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP_SUPPORT_XTENSA_XLOCALE_H
#define _LIBCPP_SUPPORT_XTENSA_XLOCALE_H
#if defined(__XTENSA__) && defined(__XCLIB__) // TENSILICA xclib

#ifdef __cplusplus
extern "C" {
#endif

# include <xlocale.h>
# include <cstdlib>
# include <clocale>
# include <cwctype>
# include <ctype.h>
# include <support/xlocale/__nop_locale_mgmt.h>
# include <support/xlocale/__posix_l_fallback.h>
# include <support/xlocale/__strtonum_fallback.h>

static inline int isascii(int c) { return c >= 0 && c < 128; }

static inline
size_t mbsnrtowcs(wchar_t * __dest, const char **__src, size_t __nms,
                  size_t __len, mbstate_t *__ps)
{
  const char *src = *__src;
  size_t src_len = __nms;
  wchar_t *dst = __dest;
  size_t dst_len = dst ? __len : (size_t) -1;
  size_t num_written = 0;
  while (dst_len > 0) {
    int src_bytes_read = mbrtowc(dst, src, src_len, __ps);
    if (src_bytes_read <= 0) {
#pragma frequency_hint never
      if (src_bytes_read == 0) {
        // completely converted
        *__src = nullptr;
        return num_written;
      }
      else if (src_bytes_read == -2) {
        // no complete multibyte character, ignore it and stop
        *__src = src + src_len;
        return num_written;
      }
      *__src = src;
      return (size_t) -1;
    }
    src += src_bytes_read;
    src_len -= src_bytes_read;
    dst_len -= 1;
    num_written += 1;
    dst = dst ? dst + 1 : dst;
  }
  *__src = src;
  return num_written;
}

static inline
size_t wcsnrtombs(char *__dest, const wchar_t **__src, size_t __nwc,
                  size_t __len, mbstate_t *__ps)
{
  const wchar_t *src = *__src;
  size_t src_len = __nwc;
  char *dst = __dest;
  size_t dst_len = dst ? __len : (size_t) -1;
  size_t num_written = 0;
  char buf[16];
  while (src_len > 0 && num_written < dst_len) {
    mbstate_t save_ps = *__ps;
    size_t bytes_to_write = wcrtomb(buf, *src, __ps);
    if (bytes_to_write == (size_t) -1) {
      return (size_t) -1;
    }
    if (num_written + bytes_to_write > dst_len) {
      *__ps = save_ps;
      return num_written;
    }
    num_written += bytes_to_write;
    if (dst) {
      for (size_t i = 0; i < bytes_to_write; ++i)
        *dst++ = buf[i];
      ++*__src;
      if (*src == 0)
        *__src = 0;
    }
    if (*src == 0) {
      return num_written - 1;
    }
    ++src;
    --src_len;
  }
  return num_written;
}

static inline
int vasprintf(char **strp, const char *fmt, va_list ap)
{
  const size_t buff_size = 256;
  int str_size;
  if ((*strp = (char *)malloc(buff_size)) == NULL)
  {
    return -1;
  }
  if ((str_size = vsnprintf(*strp, buff_size, fmt,  ap)) >= (int)buff_size)
  {
    if ((*strp = (char *)realloc(*strp, str_size + 1)) == NULL)
    {
      return -1;
    }
    str_size = vsnprintf(*strp, str_size + 1, fmt,  ap);
  }
  return str_size;
}
#ifdef __cplusplus
}
#endif
#endif // defined(__XTENSA__) && defined(__XCLIB__)
#endif // _LIBCPP_SUPPORT_XTENSA_XLOCALE_H
