
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <semphr.h>

#include <string.h>

/* v3 header */
#include <v3/fota.h>

#include "filogic.h"

#include "wifi_api_ex.h"
#include "mt7933_pos.h"

// per project header
#include "fota_flash_config.h"
#include "memory_map.h"

#define _FILOGIC_STACK_SIZE (2048)
#define _FILOGIC_PRIORITY   (configMAX_PRIORITIES - 1)
#define _SWAP(a, b) do {\
                        a = a ^ b;\
                        b = a ^ b;\
                        a = b ^ a;\
                    } while(0)

/****************************************************************************
 * LOG
 ****************************************************************************/


#define ERRR        16
#define WARN        8
#define INFO        4
#define DEBG        2
#define TRCE        1

#define E(fmt...) _filogic_log(ctx, 0, ERRR, fmt)
#define W(fmt...) _filogic_log(ctx, 0, WARN, fmt)
#define I(fmt...) _filogic_log(ctx, 0, INFO, fmt)
#define D(fmt...) _filogic_log(ctx, 0, DEBG, fmt)
#define T(fmt...) _filogic_log(ctx, 0, TRCE, fmt)

#define ENTER       T("%s enter %d\n", __func__, __LINE__)
#define LEAVE       T("%s leave %d\n", __func__, __LINE__)

#define LOG_SHOWN   (ERRR | WARN | INFO | DEBG)
//#define LOG_SHOWN   (ERRR | WARN | INFO | DEBG | TRCE)

/****************************************************************************
 * CONTEXT MAINTENANCE
 ****************************************************************************/

typedef struct filogic_ctx
{
    SemaphoreHandle_t               mutex;
    QueueHandle_t                   queue;
    filogic_logv_cbk                logv;
    filogic_async_cbk               async_cbk;
    TaskHandle_t                    task;
    bool                            over;

    filogic_wifi_opmode_t           opmode;

    bool                            scanning;
    wifi_scan_list_item_t           *ap_list;
    uint32_t                        max_ap_num;
    filogic_scan_async_cbk          scan_callback;

    bool                            wifi_sta_prov_pended;
    bool                            wifi_sta_prov_cached;
    filogic_wifi_sta_prov_t         wifi_sta_prov;

    fota_io_state_t                 *io_handle;
} _ctx_t;


bool filogic_set_logv_callback_sync(void *c, filogic_logv_cbk logv)
{
    _ctx_t *ctx = c;

    if (!ctx)
        return false;
    else
        ctx->logv = logv;

    return true;
}


static int _filogic_log(_ctx_t *ctx, int module, int level, const char *fmt, ...)
{
    int n;
    va_list ap;

    if ((LOG_SHOWN & level) == 0)
        return 0;

    va_start(ap, fmt);
    if (ctx->logv) {
        ctx->logv(module, level, fmt, ap);
        n = 0;
        //n = vprintf(fmt, ap);
    } else {
        printf("FILOGIC ");
        n = vprintf(fmt, ap);
    }
    va_end(ap);

    if (level == ERRR) {
        // delay a while to ensure error messages were printed
        vTaskDelay(100);
    }
    return n;
}


// the global pointer can't be eliminated unless Wi-Fi driver can pass the
// pointer back to us, otherwise, we need this to be able to use the context
// in callback functions
static _ctx_t *g_ctx;


static void _filogic_ctx_init(_ctx_t *ctx, int q_len, int q_item_size)
{
    memset(ctx, 0, sizeof(*ctx));

    ENTER;

    ctx->mutex = xSemaphoreCreateBinary();
    configASSERT(ctx->mutex != NULL);
    xSemaphoreGive(ctx->mutex);

    ctx->queue = xQueueCreate(q_len, q_item_size);
    configASSERT(ctx->queue != NULL);

    ctx->logv = NULL;

    ctx->io_handle = fota_malloc(sizeof(fota_io_state_t));

    LEAVE;
}


static void _filogic_ctx_deinit(_ctx_t *ctx)
{
    ENTER;

    configASSERT(ctx != NULL);

    vSemaphoreDelete(ctx->mutex);
    vQueueDelete(ctx->queue);

    LEAVE;
}


/****************************************************************************
 * JOB HANDLING
 ****************************************************************************/


typedef struct filogic_job filogic_job_t;


typedef void (*job_func_t)(_ctx_t *ctx, filogic_job_t *job);


struct filogic_job
{
    job_func_t          func;
    SemaphoreHandle_t   sync;
    void                *ret; // return value
    void                *res; // return result

    union {
        filogic_async_event_data     event_data;
        struct {
            filogic_async_event_id_t event;
            uint8_t                  *payload;
            uint32_t                 length;
        } wifi_event;
        struct {
            filogic_wifi_opmode_t   opmode;
        } wifi_init;
        struct {
            filogic_wifi_opmode_t   *opmode;
        } opmode_get;
        struct {
            filogic_wifi_opmode_t   opmode;
        } opmode_set;
        struct {
            uint8_t                 ssid[32];
            uint8_t                 ssid_len;
        } scan;
        struct {
            bool                    *link_status;
        } link_status_get;
        struct {
            filogic_wifi_sta_prov_t *prov;
            int32_t                 *err;
        } wifi_sta_prov;
        struct {
            uint8_t                 channel;
            uint8_t                 ssid[32];
            uint8_t                 ssid_len;
        } ap_config;
        struct {
            filogic_wifi_opmode_t   opmode;
            uint8_t                 *mac_addr;
        } wifi_mac_addr_get;
        struct {
            bool                    writing;
            uint32_t                partition;
            fota_io_state_t         *io_handle;
            const fota_flash_t      *flash;
        } ota;
        struct {
            const void              *addr;
            uint32_t                len;
        } ota_io;
    } u;
};


#define _FILOGIC_JOB_QLEN   (20)
#define _FILOGIC_JOB_SIZE   (sizeof(filogic_job_t))


/****************************************************************************
 * CRITICAL SECTION
 ****************************************************************************/


static void _filogic_lock(_ctx_t *ctx, const char *func)
{
    if (ctx) {
        //I("lock %s\n", func);
        BaseType_t xRet = xSemaphoreTake(ctx->mutex, portMAX_DELAY);
        configASSERT(xRet == pdPASS);
    }
}
#define LOCK() _filogic_lock(ctx, __func__)


static void _filogic_unlock(_ctx_t *ctx, const char *func)
{
    if (ctx) {
        //I("unlock %s\n", func);
        xSemaphoreGive(ctx->mutex);
    }
}
#define UNLOCK() _filogic_unlock(ctx, __func__)


/****************************************************************************
 * JOB TASK
 ****************************************************************************/


static void _job_task(void *pvParameters)
{
    BaseType_t          xRet;
    filogic_job_t       job;
    _ctx_t              *ctx;

    configASSERT(pvParameters != NULL);
    ctx = pvParameters;

    ENTER;

    do
    {
        //I("%s wait queue %x\n", __func__, ctx->queue);

        xRet = xQueueReceive( ctx->queue, &job, portMAX_DELAY );
        configASSERT( xRet == pdPASS );

        //I("%s func %x\n", __func__, job.func);

        LOCK();
        {
            job.func( ctx, &job );
            if ( job.sync != NULL ) {
                xSemaphoreGive( job.sync );
            }
        }
        UNLOCK();

    } while (!ctx->over);

    LEAVE;

    // stay here until deleted
    while (1) vTaskDelay(0x100);
}


static void job_add_sync(_ctx_t *ctx, job_func_t func, filogic_job_t *job)
{
    filogic_job_t tmp;

    //ENTER;

    if (job == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        job = &tmp;
    }

    job->func = func;
    job->sync = xSemaphoreCreateBinary();
    configASSERT( job->sync != NULL );

    configASSERT( xQueueSendToBack( ctx->queue,
                                    job,
                                    portMAX_DELAY ) == pdPASS );

    xSemaphoreTake( job->sync, portMAX_DELAY );

    vSemaphoreDelete( job->sync );

    //LEAVE;
}


static void job_add_async(_ctx_t *ctx, job_func_t func, filogic_job_t *job)
{
    filogic_job_t tmp;

    ENTER;

    if (job == NULL)
    {
        memset(&tmp, 0, sizeof(tmp));
        job = &tmp;
    }

    job->func = func;
    job->sync = NULL;
    configASSERT( xQueueSendToBack( ctx->queue,
                                    job,
                                    portMAX_DELAY ) == pdPASS );

    LEAVE;
}


/****************************************************************************
 * ID TO CORRESPONDING STRINGS
 ****************************************************************************/


const char *filogic_opmode_to_name(filogic_wifi_opmode_t opmode)
{
    const char *names[] = {
        "FILOGIC_WIFI_OPMODE_NONE",
        "FILOGIC_WIFI_OPMODE_AP",
        "FILOGIC_WIFI_OPMODE_STA",
        "FILOGIC_WIFI_OPMODE_DUAL",
    };

    configASSERT(FILOGIC_WIFI_OPMODE_MAX == sizeof(names)/sizeof(char *));

    configASSERT(opmode < FILOGIC_WIFI_OPMODE_MAX);

    return names[opmode];
}


static const char *wifi_event_to_name(wifi_event_t event)
{
    const char *names[] = {
        "WIFI_EVENT_IOT_CONNECTED",
        "WIFI_EVENT_IOT_SCAN_COMPLETE",
        "WIFI_EVENT_IOT_DISCONNECTED",
        "WIFI_EVENT_IOT_PORT_SECURE",
        "WIFI_EVENT_IOT_REPORT_BEACON_PROBE_RESPONSE",
        "WIFI_EVENT_IOT_WPS_COMPLETE",
        "WIFI_EVENT_IOT_INIT_COMPLETE",
        "WIFI_EVENT_IOT_REPORT_FILTERED_FRAME",
        "WIFI_EVENT_IOT_CONNECTION_FAILED",
        "WIFI_EVENT_IOT_WPS_COMPLETE_CONNECTION_FAILED",
        "WIFI_EVENT_IOT_ROAM_RUNNING",
        "WIFI_EVENT_IOT_IPV4_ADDR_READY",
        "WIFI_EVENT_IOT_IPV6_ADDR_READY",
        "WIFI_EVENT_IOT_CSI_DATA_NOTIFICATION",
    };

    configASSERT(WIFI_EVENT_MAX_NUMBER == sizeof(names)/sizeof(char *));

    configASSERT(event < WIFI_EVENT_MAX_NUMBER);

    return names[event];
}


const char *filogic_event_to_name(filogic_async_event_id_t event)
{
    const char *names[] = {
        "FILOGIC_START_OK",
        "FILOGIC_START_NG",
        "FILOGIC_WIFI_INIT_OK",
        "FILOGIC_SET_OPMODE_OK",
        "FILOGIC_AP_START_OK",
        "FILOGIC_AP_START_NG",
        "FILOGIC_AP_STATION_CONNECTED",
        "FILOGIC_AP_STATION_DISCONNECTED",
        "FILOGIC_STA_CONNECTED_TO_AP",
        "FILOGIC_STA_DISCONNECTED_FROM_AP",
        "FILOGIC_STA_CONNECTED_FAILED",
        "FILOGIC_STA_IPV4_ADDR_READY",
        "FILOGIC_STA_IPV6_ADDR_READY",
        "FILOGIC_SCAN_DONE",
    };

    configASSERT(FILOGIC_EVENT_ID_MAX == sizeof(names)/sizeof(char *));

    configASSERT(event < FILOGIC_EVENT_ID_MAX);

    return names[event];
}


/****************************************************************************
 * API, JOB HANDLER, EVENT HANDLER
 ****************************************************************************/


#define JOB_Fn(f) JOB_Fn_ ## f
#define EVT_Fn(f) EVT_Fn_ ## f


/****************************************************************************/


static void JOB_Fn(filogic_async_event_data_tx)(_ctx_t *ctx, filogic_job_t *job)
{
    ENTER;

    configASSERT(ctx != NULL);
    configASSERT(ctx->async_cbk != NULL);

    ctx->async_cbk(ctx, job->u.event_data.event_id, &job->u.event_data);

    LEAVE;
}


static void *filogic_async_event_data_tx(void *c,
                                         filogic_async_event_data *event_data)
{
    _ctx_t          *ctx = c;
    filogic_job_t   job = {0};

    ENTER;

    configASSERT(ctx != NULL);

    memcpy(&job.u.event_data, event_data, sizeof(job.u.event_data));
    job_add_async(ctx, JOB_Fn(filogic_async_event_data_tx), &job);

    LEAVE;

    return ctx;
}


/****************************************************************************/


static int _filogic_sta_prov_apply( _ctx_t *ctx, filogic_wifi_sta_prov_t * prov )
{
    int err;

    if ( ( err = wifi_config_set_ssid( WIFI_PORT_STA,
                                       (uint8_t *)&prov->ssid[0],
                                       (uint8_t)prov->ssid_len ) ) < 0 )
    {
        E("%s: set ssid failed %d\n", __func__, err);
        configASSERT( err >= 0 );
        return err;
    }

    if ( ( err = wifi_config_set_wpa_psk_key( WIFI_PORT_STA,
                                              (uint8_t *)&prov->psk[0],
                                              (uint8_t)prov->psk_len ) ) < 0 )
    {
        E("%s: set psk failed %d\n", __func__, err);
        configASSERT( err >= 0 );
        return err;
    }

    if ( ( err = wifi_config_set_security_mode( WIFI_PORT_STA,
                                                WIFI_AUTH_MODE_WPA2_PSK,
                                                WIFI_ENCRYPT_TYPE_AES_ENABLED ) ) < 0 )
    {
        E("%s: set sec mode failed %d\n", __func__, err);
        configASSERT( err >= 0 );
        return err;
    }

    return 0;
}


/****************************************************************************/


static void JOB_Fn(filogic_start_sync)(_ctx_t *ctx, filogic_job_t *job)
{
    ENTER;

    (void)job;
    (void)ctx;

    LEAVE;
}


void *filogic_start_sync(void)
{
    _ctx_t *ctx;

    ctx = pvPortMalloc(sizeof(*ctx));
    configASSERT(ctx != NULL);

    ENTER;

    _filogic_ctx_init(ctx, _FILOGIC_JOB_QLEN, _FILOGIC_JOB_SIZE);

    configASSERT(pdPASS == xTaskCreate(_job_task,
                                       "filogic",
                                       _FILOGIC_STACK_SIZE,
                                       (void *)ctx,
                                       _FILOGIC_PRIORITY,
                                       &ctx->task));

    job_add_sync(ctx, JOB_Fn(filogic_start_sync), NULL);

    g_ctx = ctx;

    LEAVE;

    return ctx;
}


/****************************************************************************/


static void JOB_Fn(filogic_stop_sync)(_ctx_t *ctx, filogic_job_t *job)
{
    ENTER;

    (void)job;
    ctx->over = true;

    LEAVE;
}


void filogic_stop_sync(void *c)
{
    _ctx_t          *ctx = c;

    ENTER;

    job_add_sync(ctx, JOB_Fn(filogic_stop_sync), NULL);

    _filogic_ctx_deinit(ctx);

    vTaskDelete(ctx->task);

    vPortFree(ctx);

    LEAVE;
}


/****************************************************************************/


bool filogic_set_event_callback_sync(void *c, filogic_async_cbk async_cbk)
{
    _ctx_t *ctx = c;

    ENTER;

    if (!ctx) {
        LEAVE;
        return false;
    }

    LOCK();
    {
        //I("%s: ctx %p event callback %p\n", __func__, ctx, async_cbk);
        ctx->async_cbk = async_cbk;
    }
    UNLOCK();

    LEAVE;

    return true;
}


/****************************************************************************
 *
 * Connected events of both AP and STA are dealt with the same callback
 *
 ****************************************************************************/


static int32_t EVT_Fn(filogic_wifi_connected_event_handler)(wifi_event_t event,
                                                            uint8_t *payload,
                                                            uint32_t length)
{
    _ctx_t *ctx = g_ctx;

    ENTER;

    D("event %s payload len %d\n", wifi_event_to_name(event), length);

    // MAC address (6 byte) + port (1 byte, STA = 0, AP = 1)
    configASSERT(length == 7);

    if (payload[length - 1] == 0)
    {
        filogic_async_event_data event_data = {0};

        event_data.event_id = FILOGIC_STA_CONNECTED_TO_AP;
        memcpy(&event_data.u.wifi_mac.addr[0], payload, sizeof(event_data.u.wifi_mac.addr));
        D("not forwarding\n");
        //filogic_async_event_data_tx(ctx, &event_data);

    }
    else if (payload[length - 1] == 1)
    {
        filogic_async_event_data event_data = {0};

        event_data.event_id = FILOGIC_AP_STATION_CONNECTED;
        memcpy(&event_data.u.wifi_mac.addr[0], payload, sizeof(event_data.u.wifi_mac.addr));

        filogic_async_event_data_tx(ctx, &event_data);
    }

    LEAVE;

    return 0;
}


/****************************************************************************/


static int32_t EVT_Fn(filogic_wifi_disconnected_event_handler)(wifi_event_t event,
                                                            uint8_t *payload,
                                                            uint32_t length)
{
    _ctx_t *ctx = g_ctx;

    ENTER;

    D("event %s len %d %d\n", wifi_event_to_name(event), length, payload[6]);

    configASSERT(length == 7);

    filogic_async_event_data event_data = {0};

    if (payload[6] == 0)
        event_data.event_id = FILOGIC_STA_DISCONNECTED_FROM_AP;
    else
        event_data.event_id = FILOGIC_AP_STATION_DISCONNECTED;

    memcpy(&event_data.u.wifi_mac.addr[0], payload, sizeof(event_data.u.wifi_mac.addr));
    filogic_async_event_data_tx(ctx, &event_data);

    LEAVE;

    return 0;
}


/****************************************************************************/


static int32_t EVT_Fn(filogic_wifi_connect_fail_event_handler)(wifi_event_t event,
                                          uint8_t *payload, uint32_t length)
{
    _ctx_t *ctx = g_ctx;

    ENTER;

    I("event %s len %d payload[%d %d]\n", wifi_event_to_name(event), length, payload[0], payload[1]);

    configASSERT(length == 3);

    filogic_async_event_data event_data = {0};
    event_data.event_id = FILOGIC_STA_CONNECTED_FAILED;
    filogic_async_event_data_tx(ctx, &event_data);

    LEAVE;

    return 0;
}


/****************************************************************************/


static int32_t EVT_Fn(filogic_wifi_ipv4_addr_ready_event_handler)(wifi_event_t event,
                                          uint8_t *payload, uint32_t length)
{
    _ctx_t *ctx = g_ctx;

    ENTER;

    D("event %s len %d\n", wifi_event_to_name(event), length);

    filogic_async_event_data event_data = {0};
    event_data.event_id = FILOGIC_STA_IPV4_ADDR_READY;
    memcpy(&event_data.u.ipv4_str.addr[0], payload, sizeof(event_data.u.ipv4_str.addr));
    filogic_async_event_data_tx(ctx, &event_data);

    LEAVE;

    return 0;
}


/****************************************************************************/


static int32_t EVT_Fn(filogic_wifi_ipv6_addr_ready_event_handler)(wifi_event_t event,
                                          uint8_t *payload, uint32_t length)
{
    _ctx_t *ctx = g_ctx;

    ENTER;

    D("event %s len %d\n", wifi_event_to_name(event), length);

    filogic_async_event_data event_data = {0};

    /* BUG: The reported port (AP/STA) is stored in the last byte of payload.
     *      But IPv6 address ready event always denotes itself as AP mode,
     *      regardless of actual mode, as a workaround, we use the cached
     *      opmode kept by us to determine the current mode that upper layer
     *      requested.
     *      LIMIT: if concurrent mode is used, there is no way to detemine
     *      the actual port the IPv6 address ready event is actually for.
     */
#if 0
    if (payload[length-1] == FILOGIC_WIFI_PORT_STA)
    {
        event_data.event_id = FILOGIC_STA_IPV6_ADDR_READY;
        memcpy(&event_data.u.ipv6_str.addr[0], payload, sizeof(event_data.u.ipv6_str.addr));
        filogic_async_event_data_tx(ctx, &event_data);
    }
#else
    if ( ctx->opmode == FILOGIC_WIFI_OPMODE_STA ) {
        event_data.event_id = FILOGIC_STA_IPV6_ADDR_READY;
        memcpy(&event_data.u.ipv6_str.addr[0], payload, sizeof(event_data.u.ipv6_str.addr));
        filogic_async_event_data_tx(ctx, &event_data);
    }
#endif
    else {
        D("AP mode: ipv6 event ignored\n");
    }

    LEAVE;

    return 0;
}


/****************************************************************************/


static int32_t filogic_wifi_event_handler(wifi_event_t event,
                                          uint8_t *payload, uint32_t length)
{
    filogic_async_event_data    event_data;
    _ctx_t                      *ctx = g_ctx;

    ENTER;

    switch (event) {
    case WIFI_EVENT_IOT_SCAN_COMPLETE:
        if ( ctx->scanning ) {
            event_data.event_id = FILOGIC_SCAN_DONE;
            event_data.u.scan_done.ap_list = ctx->ap_list;
            ctx->ap_list  = NULL;
            ctx->scanning = false;
            W("Send scan done\n");
            filogic_async_event_data_tx( ctx, &event_data );
        } else {
            W("Omit unsolicited scan done\n");
        }
        break;
    default:
        W("Unhandled event %s legnth %d\n", wifi_event_to_name(event), length);
        break;
    }

    LEAVE;

    return 0;
}


static int32_t EVT_Fn(filogic_wifi_init_async)(wifi_event_t event,
                                              uint8_t *payload,
                                              uint32_t length)
{
    _ctx_t *ctx = g_ctx;
    uint8_t opmode;
    int32_t ret;
    filogic_async_event_data event_data = {0};

    ENTER;

    // wifi_init payload: 1 byte to indicate port is station or ap
    configASSERT(length == 1);
    configASSERT(payload != NULL);
    configASSERT(*payload == 0 || *payload == 1);

    D("event %s %d\n", wifi_event_to_name(event), *payload);

    ret = wifi_config_get_opmode(&opmode);
    configASSERT( ret >= 0 );

    event_data.event_id = FILOGIC_WIFI_INIT_OK;

    switch (opmode)
    {
        case WIFI_MODE_STA_ONLY:
            event_data.u.wifi_init.port = FILOGIC_WIFI_PORT_STA; break;
        case WIFI_MODE_AP_ONLY:
            event_data.u.wifi_init.port = FILOGIC_WIFI_PORT_AP; break;
        default:
            configASSERT(0); break;
    }

    filogic_async_event_data_tx(ctx, &event_data);

    LEAVE;

    return 0;
}


static void JOB_Fn(filogic_wifi_init_async)(_ctx_t *ctx, filogic_job_t *job)
{
    const wifi_event_t events[] =
    {
        WIFI_EVENT_IOT_SCAN_COMPLETE,
        WIFI_EVENT_IOT_PORT_SECURE,
    };
    #define EVENTS (sizeof(events)/sizeof(wifi_event_t))

    ENTER;

    for (int i = 0; i < EVENTS; i++)
        wifi_connection_register_event_handler(events[i], filogic_wifi_event_handler);

    wifi_config_t     config      = { 0 };
    wifi_config_ext_t config_ext  = { 0 };

    switch (job->u.wifi_init.opmode) {
    case FILOGIC_WIFI_OPMODE_AP:  config.opmode = WIFI_MODE_AP_ONLY;  break;
    case FILOGIC_WIFI_OPMODE_STA: config.opmode = WIFI_MODE_STA_ONLY; break;
    default: configASSERT(0); break;
    }

    ctx->opmode = job->u.wifi_init.opmode;

    memcpy(config.ap_config.ssid, "-", 1);
    config.ap_config.ssid_length  = 1;
    config.ap_config.auth_mode    = WIFI_AUTH_MODE_OPEN;
    config.ap_config.encrypt_type = WIFI_ENCRYPT_TYPE_ENCRYPT_DISABLED;
    config.ap_config.channel      = 6;
    config.ap_config.bandwidth    = WIFI_IOT_COMMAND_CONFIG_BANDWIDTH_20MHZ;

    // Turn on connsys power
    connsys_power_on();

    wifi_connection_register_event_handler(WIFI_EVENT_IOT_CONNECTED,
                                           EVT_Fn(filogic_wifi_connected_event_handler));

    wifi_connection_register_event_handler(WIFI_EVENT_IOT_DISCONNECTED,
                                           EVT_Fn(filogic_wifi_disconnected_event_handler));

    wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE,
                                           EVT_Fn(filogic_wifi_init_async));

    wifi_connection_register_event_handler(WIFI_EVENT_IOT_CONNECTION_FAILED,
                                           EVT_Fn(filogic_wifi_connect_fail_event_handler));

    wifi_connection_register_event_handler(WIFI_EVENT_IOT_IPV4_ADDR_READY,
                                           EVT_Fn(filogic_wifi_ipv4_addr_ready_event_handler));

    wifi_connection_register_event_handler(WIFI_EVENT_IOT_IPV6_ADDR_READY,
                                           EVT_Fn(filogic_wifi_ipv6_addr_ready_event_handler));

    wifi_init(&config, &config_ext);

    LEAVE;
}


void filogic_wifi_init_async(void *c, filogic_wifi_opmode_t opmode)
{
    _ctx_t          *ctx = c;
    filogic_job_t   job  = {0};

    ENTER;

    job.u.wifi_init.opmode = opmode;
    job_add_sync(ctx, JOB_Fn(filogic_wifi_init_async), &job);

    LEAVE;
}


/****************************************************************************/


static void JOB_Fn(filogic_wifi_opmode_set_async)(_ctx_t *ctx,
                                                  filogic_job_t *job)
{
    uint8_t opmode;
    int32_t ret;

    ENTER;

    D("opmode %s\n", filogic_opmode_to_name(job->u.opmode_set.opmode));

    switch (job->u.opmode_set.opmode)
    {
        case FILOGIC_WIFI_OPMODE_STA:  opmode = WIFI_MODE_STA_ONLY; break;
        case FILOGIC_WIFI_OPMODE_AP:   opmode = WIFI_MODE_AP_ONLY;  break;
        case FILOGIC_WIFI_OPMODE_DUAL: opmode = WIFI_MODE_REPEATER; break;
        default:
            E("unsupported opmode %d\n", job->u.opmode_set.opmode); break;
            if (job->ret)
                *(bool *)job->ret = false;
            LEAVE;
            return;
    }

    if (job->ret)
        *(bool *)job->ret = true;

    ret = wifi_config_set_opmode(opmode);
    configASSERT( ret >= 0 );

    ctx->opmode = job->u.opmode_set.opmode;
    if ( ctx->opmode == FILOGIC_WIFI_OPMODE_STA ||
         ctx->opmode == FILOGIC_WIFI_OPMODE_DUAL ||
         ctx->wifi_sta_prov_pended )
    {
        int err = _filogic_sta_prov_apply( ctx, &ctx->wifi_sta_prov );
        ctx->wifi_sta_prov_pended = false;
        configASSERT( err >= 0 );
    }

    filogic_async_event_data event_data = {0};
    event_data.event_id = FILOGIC_SET_OPMODE_OK;
    event_data.u.wifi_opmode.opmode = opmode;
    filogic_async_event_data_tx(ctx, &event_data);

    LEAVE;
}


void filogic_wifi_opmode_set_async(void *c, filogic_wifi_opmode_t opmode)
{
    _ctx_t          *ctx = c;
    filogic_job_t   job = {0};

    ENTER;

    job.u.opmode_set.opmode = opmode;
    job_add_async(ctx, JOB_Fn(filogic_wifi_opmode_set_async), &job);

    LEAVE;
}


/****************************************************************************/


static void JOB_Fn(filogic_wifi_opmode_get_sync)(_ctx_t *ctx, filogic_job_t *job)
{
    uint8_t opmode;
    int32_t ret;

    //ENTER;

    ret = wifi_config_get_opmode(&opmode);
    configASSERT( ret >= 0 );

    if (job->ret)
        *(bool *)job->ret = true;

    switch (opmode)
    {
        case WIFI_MODE_STA_ONLY:
            *job->u.opmode_get.opmode = FILOGIC_WIFI_OPMODE_STA; break;
        case WIFI_MODE_AP_ONLY:
            *job->u.opmode_get.opmode = FILOGIC_WIFI_OPMODE_AP; break;
        case WIFI_MODE_REPEATER:
            *job->u.opmode_get.opmode = FILOGIC_WIFI_OPMODE_DUAL; break;
        default:
            E("unsupported opmode %d\n", opmode);
            if (job->ret)
                *(bool *)job->ret = false;
            break;
    }

    //LEAVE;
}


void filogic_wifi_opmode_get_sync(void *c, filogic_wifi_opmode_t *opmode)
{
    _ctx_t          *ctx = c;
    filogic_job_t   job = {0};

    //ENTER;

    job.u.opmode_get.opmode = opmode;
    job_add_sync(ctx, JOB_Fn(filogic_wifi_opmode_get_sync), &job);

    //LEAVE;
}


/****************************************************************************/


static void JOB_Fn(filogic_wifi_mac_addr_get_sync)(_ctx_t *ctx, filogic_job_t *job)
{
    int32_t ret = 0;

    if (job->ret)
        *(bool *)job->ret = true;

    if (!job->u.wifi_mac_addr_get.mac_addr) {
        E("wifi mac address pointer is null\n");
        *(bool *)job->ret = false;
        return;
    }

    switch (job->u.wifi_mac_addr_get.opmode) {
        case FILOGIC_WIFI_OPMODE_STA:
            ret = wifi_config_get_mac_address(WIFI_PORT_STA, job->u.wifi_mac_addr_get.mac_addr);
            break;

        case FILOGIC_WIFI_OPMODE_AP:
            ret = wifi_config_get_mac_address(WIFI_PORT_AP, job->u.wifi_mac_addr_get.mac_addr);
            break;

        default:
            E("unsupported wifi mode %d\n", job->u.wifi_mac_addr_get.opmode);
            if (job->ret)
                *(bool *)job->ret = false;
            break;
    }

    for (int i = 0; i < WIFI_MAC_ADDRESS_LENGTH / 2; ++i) {
        _SWAP(*(job->u.wifi_mac_addr_get.mac_addr + i), *(job->u.wifi_mac_addr_get.mac_addr + WIFI_MAC_ADDRESS_LENGTH - 1 - i));
    }

    configASSERT( ret >= 0 );
}


void filogic_wifi_mac_addr_get_sync(void *c, filogic_wifi_opmode_t opmode, uint8_t *addr)
{
    _ctx_t            *ctx = c;
    filogic_job_t     job = {.u.wifi_mac_addr_get.opmode = opmode,
                             .u.wifi_mac_addr_get.mac_addr = addr,
                            };

    job_add_sync(ctx, JOB_Fn(filogic_wifi_mac_addr_get_sync), &job);
}


/****************************************************************************/


void strnzcpy(void *dst, int dst_len, void *src, int src_len)
{
    char *d = dst;
    char *s = src;
    int i = 0;
    while (dst_len > 0 && src_len > 0) {
        d[i] = s[i];
        i++;
        dst_len--;
        src_len--;
    }

    if (dst_len)
        d[i] = '\0';
    else
        d[i-1] = '\0';
}


/****************************************************************************/


static void JOB_Fn(filogic_wifi_sta_prov_set_sync)(_ctx_t *ctx, filogic_job_t *job)
{
    int32_t err;
    int     n;
    int     offset;

    ENTER;

    // station provision needs to wait until STA is ready

    ctx->wifi_sta_prov = *job->u.wifi_sta_prov.prov;

    E( "ctx->opmode = %d\n", ctx->opmode );
    ctx->wifi_sta_prov_pended = ctx->opmode == FILOGIC_WIFI_OPMODE_NONE ||
                                ( ctx->opmode != FILOGIC_WIFI_OPMODE_STA &&
                                  ctx->opmode != FILOGIC_WIFI_OPMODE_DUAL );

    *job->u.wifi_sta_prov.err = ctx->wifi_sta_prov_pended ? 0 :
                                _filogic_sta_prov_apply( ctx, &ctx->wifi_sta_prov );

    LEAVE;
}


void filogic_wifi_sta_prov_set_sync(void *c, filogic_wifi_sta_prov_t *prov)
{
    _ctx_t          *ctx = c;
    filogic_job_t   job = {0};
    int32_t         wifi_err = -1;

    configASSERT( prov );

    ENTER;

    job.u.wifi_sta_prov.prov = prov;
    job.u.wifi_sta_prov.err  = &wifi_err;

    job_add_sync(ctx, JOB_Fn(filogic_wifi_sta_prov_set_sync), &job);

    LEAVE;
}


/****************************************************************************/


static void JOB_Fn(filogic_wifi_sta_prov_get_sync)(_ctx_t *ctx, filogic_job_t *job)
{
    int32_t err;
    int     n;
    int     offset;

    //ENTER;

    *job->u.wifi_sta_prov.err = 0;

    if ( ! ctx->wifi_sta_prov_cached )
    {
        err = wifi_config_get_ssid( WIFI_PORT_STA,
                                    (uint8_t *)&ctx->wifi_sta_prov.ssid[0],
                                    (uint8_t *)&ctx->wifi_sta_prov.ssid_len );
        if (err < 0) {
            E("%s: wifi_config_get_ssid failed %d\n", __func__, err);
            *job->u.wifi_sta_prov.err = err;
        }

        err = wifi_config_get_wpa_psk_key( WIFI_PORT_STA,
                                          (uint8_t *)&ctx->wifi_sta_prov.psk[0],
                                          (uint8_t *)&ctx->wifi_sta_prov.psk_len );
        if (err < 0) {
            E("%s: wifi_config_get_wpa_psk_key failed %d\n", __func__, err);
            *job->u.wifi_sta_prov.err = err;
        }

        ctx->wifi_sta_prov_cached = true;
    }

    memcpy( job->u.wifi_sta_prov.prov, &ctx->wifi_sta_prov, sizeof( ctx->wifi_sta_prov ) );

    //LEAVE;
}


bool filogic_wifi_sta_prov_get_sync(void *c, filogic_wifi_sta_prov_t *prov)
{
    _ctx_t          *ctx = c;
    filogic_job_t   job = {0};
    int32_t         wifi_err = -1;

    configASSERT( prov );

    //ENTER;

    job.u.wifi_sta_prov.prov = prov;
    job.u.wifi_sta_prov.err  = &wifi_err;

    job_add_sync(ctx, JOB_Fn(filogic_wifi_sta_prov_get_sync), &job);

    //LEAVE;

    return true;
}


/****************************************************************************/


static void JOB_Fn(filogic_wifi_ap_config_async)(_ctx_t *ctx, filogic_job_t *job)
{
    int32_t ret;
    char    buf[128];
    int     n;
    int     offset;

    ENTER;

    n = snprintf(buf, sizeof(buf), "AP config: channel %d, ssid ", job->u.ap_config.channel);
    strnzcpy(&buf[n], sizeof(buf) - n,
             &job->u.ap_config.ssid[0], job->u.ap_config.ssid_len);
    //I("%s: %s\n", __func__, buf);

    ret = wifi_config_set_channel(WIFI_PORT_AP,
                                  job->u.ap_config.channel);
    if (ret < 0)
        E("%s: wifi_config_set_channel failed %d\n", __func__, ret);

    configASSERT( ret >= 0 );

    ret = wifi_config_set_ssid(WIFI_PORT_AP,
                               &job->u.ap_config.ssid[0],
                               job->u.ap_config.ssid_len);
    if (ret < 0)
        E("%s: wifi_config_set_ssid failed %d\n", __func__, ret);

    configASSERT( ret >= 0 );

    ret = wifi_config_reload_setting();

    configASSERT( ret >= 0 );

    if (ret) {
        E("wifi_config_reload_setting failed %d\n", ret);
        ctx->async_cbk(ctx, FILOGIC_AP_START_NG, NULL);
    } else {
        filogic_async_event_data event_data = {0};
        event_data.event_id = FILOGIC_AP_START_OK;
        ctx->async_cbk(ctx, FILOGIC_AP_START_OK, &event_data);
    }

    LEAVE;
}


void filogic_wifi_ap_config_async(void *c, uint8_t channel, const char *ssid, int ssid_len)
{
    _ctx_t          *ctx = c;
    filogic_job_t   job = {0};

    ENTER;

    D("channel %d\n", channel);

    job.u.ap_config.channel  = channel;
    job.u.ap_config.ssid_len = ssid_len;
    memcpy(&job.u.ap_config.ssid[0], ssid, 32);
    job_add_async(ctx, JOB_Fn(filogic_wifi_ap_config_async), &job);

    LEAVE;
}


/****************************************************************************/


static void JOB_Fn(filogic_wifi_scan)(_ctx_t *ctx, filogic_job_t *job)
{
    int32_t ret;
    char    buf[128];
    int     n;
    int     offset;

    ENTER;

    do {
        ret = wifi_connection_scan_init( ctx->ap_list, ctx->max_ap_num );
        if ( ret < 0 ) {
            E("%s: wifi_connection_scan_init FAIL %d\n", __func__, ret );
            break;
        }

        ret = wifi_connection_start_scan( &job->u.scan.ssid[0],
                                          job->u.scan.ssid_len,
                                          NULL,
                                          0,   // full scan
                                          0 ); // active scan
        if ( ret < 0 ) {
            E("%s: wifi_connection_start_scan FAIL %d\n", __func__, ret );
            break;
        }
    } while (0);

    if ( ret < 0 ) {
        ctx->scanning = false;
        vPortFree( ctx->ap_list );
    }
    configASSERT( ret >= 0 );

    LEAVE;
}


void filogic_wifi_scan(void                     *c,
                       const uint8_t            *ssid,
                       uint8_t                  ssid_len,
                       uint8_t                  max_ap_num,
                       filogic_scan_async_cbk   callback)
{
    _ctx_t          *ctx = c;
    filogic_job_t   job = {0};

    ENTER;

    D("Wi-Fi scan for %s\n", ssid);

    if (ctx->scanning) {
        E("Wi-Fi already scanning\n");
    } else {
        size_t size = max_ap_num * sizeof( wifi_scan_list_item_t );

        ctx->scanning      = true;
        ctx->max_ap_num    = max_ap_num;
        ctx->scan_callback = callback;
        ctx->ap_list       = pvPortMalloc( size );
        configASSERT( ctx->ap_list );

        job.u.scan.ssid_len = ssid_len;
        memcpy(&job.u.scan.ssid[0], ssid, ssid_len + 1);
        job_add_async(ctx, JOB_Fn(filogic_wifi_scan), &job);
    }

    LEAVE;
}



/****************************************************************************/




static void JOB_Fn(filogic_wifi_sta_get_link_status_sync)(_ctx_t *ctx, filogic_job_t *job)
{
    int32_t         err;
    uint8_t         status;

    (void)ctx;

    ENTER;

    *job->u.link_status_get.link_status = false;

    err = wifi_connection_get_link_status( &status );
    if (err < 0) {
        E("wifi_connection_get_link_status failed\n"); // ERROR
    } else if ( status == WIFI_STATUS_LINK_CONNECTED ) {
        *job->u.link_status_get.link_status = true;
    }

    LEAVE;
}


bool filogic_wifi_sta_get_link_status_sync(void *c)
{
    _ctx_t          *ctx = c;
    filogic_job_t   job = {0};
    bool            linked;

    ENTER;

    job.u.link_status_get.link_status = &linked;
    job_add_sync(ctx, JOB_Fn(filogic_wifi_sta_get_link_status_sync), &job);

    LEAVE;

    return linked;
}


static void JOB_Fn(filogic_ota_init_sync)(_ctx_t *ctx, filogic_job_t *job)
{
    int32_t         err;
    bool            isComplete = true;
    char *block = NULL;
    (void)ctx;

    ENTER;

    if (job->ret) *(filogic_ota_state_t *)job->ret = FILOGIC_OTA_SUCCESS;

    do {
        if (!job->u.ota.io_handle) {
           isComplete = false;
           break;
        }

        block = fota_malloc(job->u.ota.flash->block_size);
        if (!block) {
           isComplete = false;
           break;
        } else
            memset(block, 0, sizeof(job->u.ota.flash->block_size));

        if (fota_io_init(job->u.ota.flash, job->u.ota.partition, job->u.ota.io_handle) != FOTA_STATUS_OK) {
            isComplete = false;
            break;
        }

        if (fota_io_seek(job->u.ota.io_handle, 0) != FOTA_STATUS_OK) {
            isComplete = false;
            break;
        }

        // clear header
        if (fota_io_write(job->u.ota.io_handle, block,
                    job->u.ota.flash->block_size) != FOTA_STATUS_OK) {
            isComplete = false;
            break;
        }

        if (fota_io_seek(job->u.ota.io_handle,
                    - job->u.ota.flash->block_size) != FOTA_STATUS_OK) {
            isComplete = false;
            break;
        }

        // clear info block
        if (fota_io_write(job->u.ota.io_handle, block,
                    job->u.ota.flash->block_size) != FOTA_STATUS_OK) {
            isComplete = false;
            break;
        }

        fota_free(block);
        block = NULL;

        if (fota_io_seek(job->u.ota.io_handle, 0) != FOTA_STATUS_OK) {
            isComplete = false;
            break;
        }
    } while (0);

    if (!isComplete) {
        if (job->ret) *(filogic_ota_state_t *)job->ret = FILOGIC_OTA_INIT_FAIL;
    }

    LEAVE;
}


void filogic_ota_init_sync(void *c, void *ret)
{
    _ctx_t          *ctx = c;
    filogic_job_t   job = {.ret = ret,
                           .u.ota.partition = ROM_REGION_FOTA,
                           .u.ota.io_handle = ctx->io_handle,
                           .u.ota.flash = &g_fota_flash_config};

    ENTER;

    job_add_sync(ctx, JOB_Fn(filogic_ota_init_sync), &job);

    if (job.ret) {
       if (*(filogic_ota_state_t *)job.ret != FILOGIC_OTA_SUCCESS) {
           E("OTA init failed\n"); // ERROR
       } else {
           I("OTA init success\n");
       }
    }

    LEAVE;
}


static void JOB_Fn(filogic_ota_io_write_sync)(_ctx_t *ctx, filogic_job_t *job)
{
    ENTER;
    I("ota io write 0x%x into 0x%x, len: %d, bus addr: 0x%x, offset: 0x%x\n", job->u.ota_io.addr, ctx->io_handle->phy_addr, job->u.ota_io.len, ctx->io_handle->bus_addr, ctx->io_handle->offset);
    if (fota_io_write(ctx->io_handle, job->u.ota_io.addr, job->u.ota_io.len) != FOTA_STATUS_OK) {
        E("fota io write fail\n");
        if (job->ret) *(filogic_ota_state_t *)job->ret = FILOGIC_OTA_WRITE_FAIL;
    } else {
        I("fota io write success\n");
        if (job->ret) *(filogic_ota_state_t *)job->ret = FILOGIC_OTA_SUCCESS;
    }
    LEAVE;
}


void filogic_ota_io_write_sync(void *c, const void *addr, uint32_t len, void *ret)
{
    _ctx_t          *ctx = c;
    filogic_job_t   job = {.ret = ret,
                           .u.ota_io.addr = addr,
                           .u.ota_io.len = len};
    ENTER;
    job_add_sync(ctx, JOB_Fn(filogic_ota_io_write_sync), &job);
    LEAVE;
}


static void JOB_Fn(filogic_ota_apply_sync)(_ctx_t *ctx, filogic_job_t *job)
{
    ENTER;
    if (job->u.ota.flash) {
        if (fota_trigger_upgrade(job->u.ota.flash, ROM_REGION_FOTA) == FOTA_STATUS_OK) {
            I("FOTA trigger upgrade success\n");
            if (job->ret) *(filogic_ota_state_t *)job->ret = FILOGIC_OTA_SUCCESS;
        } else {
            E("FOTA trigger upgrade fail\n");
            if (job->ret) *(filogic_ota_state_t *)job->ret = FILOGIC_OTA_APPLY_FAIL;
        }
    } else {
        E("FOTA trigger upgrade fail\n");
        if (job->ret) *(filogic_ota_state_t *)job->ret = FILOGIC_OTA_UNKNOWN_FAIL;
    }

    LEAVE;
}


void filogic_ota_apply_sync(void *c, void *ret)
{
    _ctx_t          *ctx = c;
    filogic_job_t   job = {.ret = ret,
                           .u.ota.partition = ROM_REGION_FOTA,
                           .u.ota.flash = &g_fota_flash_config};

    ENTER;
    job_add_sync(ctx, JOB_Fn(filogic_ota_apply_sync), &job);
    LEAVE;
}


/****************************************************************************/


// for SDK modules
void filogic_log_print(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if (g_ctx && g_ctx->logv) {
        g_ctx->logv(0, INFO, fmt, ap);
    } else {
        printf("FILOGIC ");
        (void)vprintf(fmt, ap);
    }
    va_end(ap);
}
