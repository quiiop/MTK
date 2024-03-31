#ifndef _CI_H_
#define _CI_H_

#include "hal.h"
#include "ci_cli.h"

#define EXPECT_VAL_MASK(_val, _mask, _expected) \
    { \
        unsigned int __val = (unsigned int)(_val); \
        unsigned int __expect = (unsigned int)(_expected); \
        if ((__val & (_mask)) != __expect) { \
            printf("FUNC:%s, LINE:%d, val:%08x, mask:%08x, expect:%08x\n", __func__, __LINE__, __val, (_mask), __expect); \
            return CI_FAIL; \
        } \
    }

#define EXPECT_VAL(_val, _expected) \
        { \
            unsigned int __val = (unsigned int)(_val); \
            unsigned int __expect = (unsigned int)(_expected); \
            if (__val != __expect) { \
                printf("FUNC:%s, LINE:%d, val:%08x, expect:%08x\n", __func__, __LINE__, __val, __expect); \
                return CI_FAIL; \
            } \
        }

#define EXPECT_NOT_VAL(_val, _not_expected) \
        { \
            unsigned int __val = (unsigned int)(_val); \
            unsigned int __not_expect = (unsigned int)(_not_expected); \
            if (__val == __not_expect) { \
                printf("FUNC:%s, LINE:%d, val:%08x, notexpect:%08x\n", __func__, __LINE__, __val, __not_expect); \
                return CI_FAIL; \
            } \
        }

#define EXPECT_VAL_RANGE(_val, _min, _max) \
        { \
            unsigned int __val = (unsigned int)(_val); \
            unsigned int __min = (unsigned int)(_min); \
            unsigned int __max = (unsigned int)(_max); \
            if (__val < __min || __val > __max) { \
                printf("FUNC:%s, LINE:%d, val:%u, not in range %u-%u\n", __func__, __LINE__, __val, __min, __max); \
                return CI_FAIL; \
            } \
        }

#define CI_UT_MAIN(_IPNAME)


#define FUNCTION_NAME(IPNAME) \
    ci_status_t ci_##IPNAME##_sample_main(unsigned int portnum); \
    CI_UT_MAIN(IPNAME)

struct test_entry {
    char *test_name;
    ci_status_t (*test_func)(void);
};

ci_status_t test_execution(struct test_entry *case_list, unsigned int num_case);


#ifdef CI_SDIOM_ENABLE
FUNCTION_NAME(sdiom)
#endif /* #ifdef CI_SDIOM_ENABLE */

#ifdef CI_SDIOS_ENABLE
FUNCTION_NAME(sdios)
#endif /* #ifdef CI_SDIOS_ENABLE */

#ifdef CI_USB_GADGET_ENABLE
FUNCTION_NAME(usb_gadget)
#endif /* #ifdef CI_USB_GADGET_ENABLE */

#ifdef CI_USB_HOST_ENABLE
FUNCTION_NAME(usb_host)
#endif /* #ifdef CI_USB_HOST_ENABLE */

#ifdef CI_I2C_ENABLE
FUNCTION_NAME(i2c)
#endif /* #ifdef CI_I2C_ENABLE */

#ifdef CI_SPI_ENABLE
FUNCTION_NAME(spi)
#endif /* #ifdef CI_SPI_ENABLE */

#ifdef CI_RTC_ENABLE
FUNCTION_NAME(rtc)
#endif /* #ifdef CI_RTC_ENABLE */

#ifdef CI_EINT_ENABLE
FUNCTION_NAME(eint)
#endif /* #ifdef CI_EINT_ENABLE */

#ifdef CI_GCPU_ENABLE
FUNCTION_NAME(gcpu)
#endif /* #ifdef CI_GCPU_ENABLE */

#ifdef CI_ECC_ENABLE
FUNCTION_NAME(ecc)
#endif /* #ifdef CI_ECC_ENABLE */

#ifdef CI_GDMA_ENABLE
FUNCTION_NAME(gdma)
#endif /* #ifdef CI_GDMA_ENABLE */

#ifdef CI_ADC_ENABLE
FUNCTION_NAME(adc)
#endif /* #ifdef CI_ADC_ENABLE */

#ifdef CI_NVIC_ENABLE
FUNCTION_NAME(nvic)
#endif /* #ifdef CI_NVIC_ENABLE */

#ifdef CI_PWM_ENABLE
FUNCTION_NAME(pwm)
#endif /* #ifdef CI_PWM_ENABLE */

#ifdef CI_KEYPAD_ENABLE
FUNCTION_NAME(keypad)
#endif /* #ifdef CI_KEYPAD_ENABLE */

#ifdef CI_IRRX_ENABLE
FUNCTION_NAME(irrx)
#endif /* #ifdef CI_IRRX_ENABLE */

#ifdef CI_GPT_ENABLE
FUNCTION_NAME(gpt)
#endif /* #ifdef CI_GPT_ENABLE */

#ifdef CI_WDT_ENABLE
FUNCTION_NAME(wdt)
#endif /* #ifdef CI_WDT_ENABLE */

#ifdef CI_SLEEPMANAGER_ENABLE
FUNCTION_NAME(sleepmanager)
#endif /* #ifdef CI_SLEEPMANAGER_ENABLE */

#ifdef CI_PMU_ENABLE
FUNCTION_NAME(pmu)
#endif /* #ifdef CI_PMU_ENABLE */

#ifdef CI_NVDM_ENABLE
FUNCTION_NAME(nvdm)
#endif /* #ifdef CI_NVDM_ENABLE */

#ifdef CI_CACHE_ENABLE
FUNCTION_NAME(cache)
#endif /* #ifdef CI_CACHE_ENABLE */

#ifdef CI_MPU_ENABLE
FUNCTION_NAME(mpu)
#endif /* #ifdef CI_MPU_ENABLE */

#ifdef CI_GPIO_ENABLE
FUNCTION_NAME(gpio)
#endif /* #ifdef CI_GPIO_ENABLE */

#ifdef CI_FLASH_ENABLE
FUNCTION_NAME(flash)
#endif /* #ifdef CI_FLASH_ENABLE */

#ifdef CI_SD_ENABLE
FUNCTION_NAME(sd)
#endif /* #ifdef CI_SD_ENABLE */

#ifdef CI_TRNG_ENABLE
FUNCTION_NAME(trng)
#endif /* #ifdef CI_TRNG_ENABLE */

#ifdef CI_I2S_ENABLE
FUNCTION_NAME(i2s)
#endif /* #ifdef CI_I2S_ENABLE */

#endif /* #ifndef _CI_H_ */
