#ifndef _AUD_LOG_H_
#define _AUD_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


#define AUD_DRV_SYSLOG

#ifdef AUD_DRV_SYSLOG
#include "syslog.h"
#endif /* #ifdef AUD_DRV_SYSLOG */


enum AUD_LOG_LEVEL {
    AUD_LOG_ERR = 1,
    AUD_LOG_WARN,
    AUD_LOG_MSG,
    AUD_LOG_DBG,
    AUD_LOG_DEV,
};

void set_loglevel(enum AUD_LOG_LEVEL level);

enum AUD_LOG_LEVEL get_loglevel(void);

#ifdef AUD_DRV_SYSLOG

#define aud_msg(...) do { if (get_loglevel() >= AUD_LOG_MSG) LOG_I(AUD_DRV, __VA_ARGS__); } while (0)
#define aud_error(...) do { if (get_loglevel() >= AUD_LOG_ERR) LOG_E(AUD_DRV, __VA_ARGS__); } while (0)
#define aud_dbg(...) do { if (get_loglevel() >= AUD_LOG_DBG) LOG_I(AUD_DRV, __VA_ARGS__); } while (0)
#define aud_dev(...) do { if (get_loglevel() >= AUD_LOG_DEV) LOG_I(AUD_DRV, __VA_ARGS__); } while (0)

#else /* #ifdef AUD_DRV_SYSLOG */
void do_print(const char *fcn, long line, const char *fmt, ...);

#define aud_msg(...) do { if (get_loglevel() >= AUD_LOG_MSG) do_print(__FUNCTION__, __LINE__, "msg: "__VA_ARGS__); } while (0)
#define aud_error(...) do { if (get_loglevel() >= AUD_LOG_ERR) do_print(__FUNCTION__, __LINE__, "error: "__VA_ARGS__); } while (0)
#define aud_dbg(...) do { if (get_loglevel() >= AUD_LOG_DBG) do_print(__FUNCTION__,__LINE__, "dbg: "__VA_ARGS__); } while (0)
#define aud_dev(...) do { if (get_loglevel() >= AUD_LOG_DEV) do_print(__FUNCTION__,__LINE__, "dev: "__VA_ARGS__); } while (0)

#endif /* #ifdef AUD_DRV_SYSLOG */



#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifndef _AUD_LOG_H_ */


