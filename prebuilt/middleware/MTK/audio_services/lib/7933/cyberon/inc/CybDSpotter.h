#ifndef __DSPOTTER_CONTROL_H
#define __DSPOTTER_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

#define CYB_SUCCESS               0
#define CYB_ERROR                -1
#define CYB_INVALID_PARAM        -2
#define CYB_LICENSE_NOT_EXIST    -3
#define CYB_FLASH_ERROR          -4
#define CYB_GET_LICENSE_FAIL     -5
#define CYB_CHECK_LICENSE_FAIL   -6
#define CYB_INSUFFICIENT_MEMORY  -7
#define CYB_INVALID_CHIP         -8

#define MAX_COMMAND_TIME         5000   // Command must be spoke within this time, default setting is 5000 ms.
#define COMMAND_STAGE_TIMEOUT    6000   // When there is no recogized result after this time at command stage, it will switch to trigger stage.

typedef struct _DSpotterControl {
    int16_t nMaxCommandTime;            // Command must be spoke within this time(ms).
    int16_t nCommandStageTimeout;       // The minimum recording time in ms when there is no result at command stage.
    // The reasonable range is sMaxCommandTime to 10000 ms.
    uint8_t nCommandStageFlowControl;   // If 0, the recognition flow will switch to trigger stage immediately after
    // command recognized. If 1, it will recognize repeatedly at command stage
    // till to timeout.
    uint8_t byAGC_Gain;
    uint8_t byaReserveSetting[6];       // The model selection information may put here.
}   DSpotterControl;

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/* Check the DSpotter license data on the flash.
 * Return CYB_SUCCESS or CYB_LICENSE_NOT_EXIST. */
int CybDSpotterCheckLicense(void);

/* Download the DSpotter license from server then save to flash.
 *   pCustomerAuthorizedData(in): The authorized data, it is different for each customer.
 *   nAuthorizedDataSize(in): The size of authorized data.
 * Return CYB_SUCCESS or other error value. */
int CybDSpotterGetLicense(const uint8_t *pCustomerAuthorizedData, const int nAuthorizedDataSize);

/* Initial DSpotter recognition.
 *   pDSpotterControl(in): The settings to control recognition flow.
 * Return CYB_SUCCESS or other error value. */
int CybDSpotterInit(const DSpotterControl *pDSpotterControl);

/* Initial DSpotter recognition.
 *   pDSpotterControl(in): The settings to control recognition flow.
 *   pCustomerAuthorizedData(in): The authorized data, it is different for each customer.
 *   nAuthorizedDataSize(in): The size of authorized data.
 * Return CYB_SUCCESS or other error value. */
int CybDSpotterInitEx(const DSpotterControl *pDSpotterControl, const uint8_t *pCustomerAuthorizedData, const int nAuthorizedDataSize);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */


#endif /* #ifndef __DSPOTTER_CONTROL_H */
