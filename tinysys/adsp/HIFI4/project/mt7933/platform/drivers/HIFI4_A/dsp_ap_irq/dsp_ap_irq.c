#include "dsp_ap_irq.h"
#include "dsp_state.h"
#include "interrupt.h"
#include "mt_printf.h"
#include "driver_api.h"

#include "systimer.h"
#include "semphr.h"
#include "cli.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/***************************************************************
                                                    Macro
***************************************************************/

#define ENABLE_INT_TO_AP(BIT) \
    do { \
        DRV_WriteReg32_Mask(RG_DSPIRQS_DSP_TO_CM33_INT_EN, 0x1 << BIT, 0x1 << BIT); \
    } while (0)

#define DISABLE_INT_TO_AP(BIT) \
    do { \
        DRV_WriteReg32_Mask(RG_DSPIRQS_DSP_TO_CM33_INT_EN, 0x0 << BIT, 0x1 << BIT); \
    } while (0)

#define TRIGGER_INT_TO_AP(BIT) \
    do { \
        DRV_WriteReg32_Mask(RG_DSPIRQS_DSP_TO_CM33_INT_TRIG, 0x1 << BIT, 0x1 << BIT); \
    } while (0)

#define ENABLE_INT_FROM_AP(BIT) \
    do { \
        DRV_WriteReg32_Mask(RG_DSPIRQS_CM33_TO_DSP_INT_EN, 0x1 << BIT, 0x1 << BIT); \
    } while (0)

#define DISABLE_INT_FROM_AP(BIT) \
    do { \
        DRV_WriteReg32_Mask(RG_DSPIRQS_CM33_TO_DSP_INT_EN, 0x0 << BIT, 0x1 << BIT); \
    } while (0)

#define CLEAR_INT_FROM_AP(BIT) \
    do { \
        DRV_WriteReg32_Mask(RG_DSPIRQS_CM33_TO_DSP_INT_STS, 0x1 << BIT, 0x1 << BIT); \
    } while (0)

#define TRIGGER_CPU_TO_AP(BIT) \
    do { \
        DRV_WriteReg32_Mask(RG_INT2CIRW, 0x1 << BIT, 0x1 << BIT); \
    } while (0)
/***************************************************************
                          Globle
***************************************************************/
struct ap2dsp_irq_data {
    void (*irq_callback)(void);
};

unsigned int psram_ready = 1;
unsigned int ap_ready = 1;
static SemaphoreHandle_t xSemaphore_ipi = NULL;

#ifdef DSP_STATE_SUPPORT
static TaskHandle_t monitor_handler;
static QueueHandle_t monitor_queue;
#endif

/***************************************************************
                        AP request
***************************************************************/

/* Should make sure psram no longer use before calling this */
static int dsp2ap_request_sleep_psram(void)
{
    TRIGGER_INT_TO_AP(INT_SLEEP_PSRAM);
    return 0;
}

/* Should use psram after this request done */
static int dsp2ap_request_wakeup_psram(void)
{
    unsigned long long time_start, time_end;
    int ret = 0;

//    TRIGGER_INT_TO_AP(INT_WAKEUP_PSRAM2);
    TRIGGER_INT_TO_AP(INT_WAKEUP_PSRAM);
    TRIGGER_CPU_TO_AP(1);
    time_start = read_systimer_stamp_ns();
    time_end = time_start;
    while (psram_ready == 0 || ap_ready == 0){
        vTaskDelay(1/portTICK_PERIOD_MS);
        time_end = read_systimer_stamp_ns();

        if (time_end - time_start > 20000000) {
            ret = -1;
            PRINTF_E("request_wakeup_psram timeout:%llu\n",  (time_end - time_start));
            break;
        }
    }
    PRINTF_E("request_wakeup_psram total time:%llu\n",  (time_end - time_start));
    return ret;
}

/***************************************************************
                     AP IRQ handle
***************************************************************/
static void irq_callback_notify_psram_sleeped(void)
{
    psram_ready = 0;
    PRINTF_D("[%s].\n", __func__);
}

static void irq_callback_notify_psram_wakeuped(void)
{
    psram_ready = 1;
    PRINTF_D("[%s].\n", __func__);
}

static void irq_callback_ack_ap_suspend(void)
{
    unsigned int id = 0;
    BaseType_t woken = pdFALSE;
    ap_ready = 0;
/*
    xSemaphoreTakeFromISR(xSemaphore_ipi, NULL);
    TRIGGER_INT_TO_AP(INT_AP_SUSPEND);
    xSemaphoreGiveFromISR(xSemaphore_ipi, NULL);
*/
    vTaskNotifyGiveFromISR(monitor_handler, &woken);
    portYIELD_FROM_ISR(woken);

    xQueueSendToBackFromISR(monitor_queue, &id, &woken);
    portYIELD_FROM_ISR(woken);

    PRINTF_D("[%s].\n", __func__);
}

static void irq_callback_ack_ap_resume(void)
{
    unsigned int id = 1;
    BaseType_t woken = pdFALSE;
    ap_ready = 1;
/*
    xSemaphoreTakeFromISR(xSemaphore_ipi, NULL);
    TRIGGER_INT_TO_AP(INT_AP_RESUME);
    xSemaphoreGiveFromISR(xSemaphore_ipi, NULL);
*/
    vTaskNotifyGiveFromISR(monitor_handler, &woken);
    portYIELD_FROM_ISR(woken);

    xQueueSendToBackFromISR(monitor_queue, &id, &woken);
    portYIELD_FROM_ISR(woken);

    PRINTF_D("[%s].\n", __func__);
}

static struct ap2dsp_irq_data irq_data[INT_NUM] = {
    [INT_SLEEP_PSRAM] = {
        .irq_callback = irq_callback_notify_psram_sleeped,
    },
    [INT_WAKEUP_PSRAM] = {
        .irq_callback = irq_callback_notify_psram_wakeuped,
    },
    [INT_AP_SUSPEND] = {
        .irq_callback = irq_callback_ack_ap_suspend,
    },
    [INT_AP_RESUME] = {
        .irq_callback = irq_callback_ack_ap_resume,
    },
};


/* CM33 to DSP irq handle */
static void ap2dsp_irq_handler(void)
{
    unsigned int status;
    int i;

    status = DRV_Reg32(RG_DSPIRQS_CM33_TO_DSP_INT_STS);
    for (i = 0; i < INT_NUM; i++) {
        if (((status & 1U << (i)) != 0) &&
            (irq_data[i].irq_callback != NULL)) {
            irq_data[i].irq_callback();
        }
    }

    /* clear irq status */
    DRV_WriteReg32(RG_DSPIRQS_CM33_TO_DSP_INT_STS, status);
}

#ifdef DSP_STATE_SUPPORT
static void ap_ds_monitor_handle_loop(void* data)
{
    unsigned int id;

    while (1) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        xQueueReceive(monitor_queue, &id, 0);

        switch (id) {
        case 0:
            dsp_state_event_proc(DSP_EVT_AP_SUSPEND);
            break;
        case 1:
            dsp_state_event_proc(DSP_EVT_AP_RESUME);
            break;
        default:
            PRINTF_E("%s, msgID[%d] not implement yet.\n", __func__, id);
            break;
        }
    }
}
#endif
/***************************************************************
                            INIT & API
***************************************************************/

int ap_req_init(void)
{
    int i = 0;
    /* IRQ mask enable */
    for (i = 0; i < INT_NUM; i++)
        ENABLE_INT_TO_AP(i);
    for (i = 0; i < INT_NUM; i++)
        ENABLE_INT_FROM_AP(i);

    /*register irq handler*/
    request_irq(CM33_IRQn, ap2dsp_irq_handler, "cm33_irq_Handle");
    xSemaphore_ipi = xSemaphoreCreateBinary();

#ifdef DSP_STATE_SUPPORT
    xTaskCreate(
        ap_ds_monitor_handle_loop,
        "AP deep sleep monitor handle",
        configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 2,
        &monitor_handler);
    monitor_queue = xQueueCreate(20, sizeof(unsigned int));
#endif

    return 0;
}

int ap_req_uninit(void)
{
    int i = 0;
    free_irq(CM33_IRQn);
    /* IRQ mask disable */
    for (i = 0; i < INT_NUM; i++)
        DISABLE_INT_TO_AP(i);
    for (i = 0; i < INT_NUM; i++)
        DISABLE_INT_FROM_AP(i);
    return 0;
}

int ap_req_set(int req)
{
    int ret = 0;

    switch (req) {
    case INT_SLEEP_PSRAM:
        ret = dsp2ap_request_sleep_psram();
        break;
    case INT_WAKEUP_PSRAM:
        ret = dsp2ap_request_wakeup_psram();
        break;
    default:
        PRINTF_E("[%s] unhandled request.", __func__);
        break;
    }

    if (ret != 0)
        PRINTF_E("[%s] Err happened when request(%d).\n", __func__, req);

    return ret;
}

int ap_req_get(void)
{
    return 0;
}

