/*
 * wpa_supplicant/hostapd - Default include files
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This header file is included into all C files so that commonly used header
 * files can be selected with OS specific ifdef blocks in one place instead of
 * having to have OS/C library specific selection in many files.
 */

#ifndef INCLUDES_H
#define INCLUDES_H

/* Include possible build time configuration before including anything else */
#include "build_config.h"
#include "wfa_debug.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifndef _WIN32_WCE
#ifndef _FREERTOS
#include <signal.h>
#endif
#include <sys/types.h>
#include <errno.h>
#endif /* _WIN32_WCE */
#include <ctype.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif /* _MSC_VER */

#ifndef CONFIG_NATIVE_WINDOWS
#ifndef _FREERTOS
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <fcntl.h>
#include "lwip/sockets.h"
#endif

#ifndef __vxworks
#ifndef _FREERTOS
#include <sys/uio.h>
#endif
#include <sys/time.h>
#endif /* __vxworks */
#endif /* CONFIG_NATIVE_WINDOWS */

#endif /* INCLUDES_H */
