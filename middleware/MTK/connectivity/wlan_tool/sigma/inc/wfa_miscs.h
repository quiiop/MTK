/****************************************************************************
*
* Copyright (c) 2016 Wi-Fi Alliance
*
* Permission to use, copy, modify, and/or distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
* SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
* RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
* NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
* USE OR PERFORMANCE OF THIS SOFTWARE.
*
*****************************************************************************/


#ifndef _WFA_MISCS_H_
#define _WFA_MISCS_H_

#ifdef _FREERTOS
#include <stdio.h>
#ifdef MTK_MINISUPP_ENABLE
#include "utils/os.h"
#endif
#endif


extern int isString(char *);
extern int isNumber(char *);
extern int isIpV4Addr(char *);
#ifndef _FREERTOS
extern inline double wfa_timeval2double(struct timeval *tval);
extern inline void wfa_double2timeval(struct timeval *tval, double dval);
#else
#ifdef MTK_MINISUPP_ENABLE
static inline double wfa_timeval2double(struct os_time *tval)
{
    return ((double) tval->sec + (double) tval->usec*1e-6);
}

static inline void wfa_double2timeval(struct os_time *tval, double dval)
{
    tval->sec = (long int) dval;
    tval->usec = (long int) ((dval - tval->sec) * 1000000);
}
#endif
#endif
#ifndef _FREERTOS
#ifdef CONFIG_MTK_COMMON
extern double wfa_ftime_diff(struct timeval *t1, struct timeval *t2);
#else
extern inline double wfa_ftime_diff(struct timeval *t1, struct timeval *t2);
#endif
#else
#ifdef MTK_MINISUPP_ENABLE
#ifdef CONFIG_MTK_COMMON
extern double wfa_ftime_diff(struct os_time *t1, struct os_time *t2);
#else
extern inline double wfa_ftime_diff(struct os_time *t1, struct os_time *t2);
#endif
#endif
#endif

#ifndef _FREERTOS
extern int wfa_itime_diff(struct timeval *t1, struct timeval *t2);
#else
#ifdef MTK_MINISUPP_ENABLE
extern int wfa_itime_diff(struct os_time *t1, struct os_time *t2);
#endif
#endif
extern void int2BuffBigEndian(int val, char *buf);
extern int bigEndianBuff2Int(char *buff);

#endif
