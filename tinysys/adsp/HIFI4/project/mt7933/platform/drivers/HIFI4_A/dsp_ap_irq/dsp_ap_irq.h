#ifndef __CM33_H__
#define __CM33_H__

/* Respect to each bit of DSP/CM33 IRQ register */
enum {
    INT_SLEEP_PSRAM = 0,
    INT_WAKEUP_PSRAM = 1,
    INT_AP_SUSPEND = 2,
    INT_AP_RESUME = 3,
    INT_WAKEUP_PSRAM2 = 4,
    INT_NUM,
};

int ap_req_set(int req);
int ap_req_get(void);

int ap_req_init(void);
int ap_req_uninit(void);


#endif /* __CM33_H__ */
