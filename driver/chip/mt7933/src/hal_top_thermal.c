/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */
#include "common.h"
#include "hal.h"
#include "hal_top_thermal.h"
#include "hal_gpt.h"
#include "driver_api.h"
#include "hal_spm.h"

//==================================================================================================
// MTCMOS pwr_on/down
//==================================================================================================

#define READ_EFUSE_FAIL_PATTERN_STR "Read efuse 0x%2X fail !\r\n"

#if defined(HAL_TOP_THERMAL_MODULE_ENABLED)

#define Default_Slope_molecular         204
#define Default_Slope_denominator       100
#define Defalut_offset                  28
#define Default_compensation           0x39
int slope = Default_Slope_molecular;
int offset = Defalut_offset;
int compensation = Default_compensation;

int efuse_logical_read_8bits(uint32_t addr, uint8_t *val)
{
    uint32_t addr_align32 = addr & 0xf0;
    uint32_t _offset = addr & 0x0f;
    uint32_t ret;
    long unsigned int efuse_buf[4];
    ret = efuse_logical_read(0x1, addr_align32, efuse_buf);
    if (ret) {
        //log_hal_error("failed to read efuse\r\n");
        return -1;
    }

    *val = efuse_buf[_offset / 4] >> ((_offset % 4) * 8);
    *val = *val & 0xff;
    return 0;

}

int adie_thermal_init(void)
{
    int ret = 0;
    uint8_t val = 0;
    uint8_t valid_bit = 0;
    uint8_t VRPI_SEL = 0;
    uint8_t THADC_PGA_GAIN = 0;
    uint8_t RG_THADC_VREF_SEL = 0;
    uint32_t adieCR34 =  0x0;
    uint32_t adieCR38 =  0x0;
    // TOP_THADC_ANA_CAL(0xAA)
    ret = efuse_logical_read_8bits(0xAA, &val);
    if (ret) {
        log_hal_error(READ_EFUSE_FAIL_PATTERN_STR, 0xAA);
        return -1;
    }
    valid_bit = RET_SPECIAL_BITS(val, MASK_PATTERN7, 7);
    VRPI_SEL =  RET_SPECIAL_BITS(val, MASK_PATTERN3_6, 3);
    THADC_PGA_GAIN = RET_SPECIAL_BITS(val, MASK_PATTERN2_0, 0);

    if (valid_bit) {
        adieCR34 = topspi_read(0x34, 0x5);
        adieCR38 = topspi_read(0x38, 0x5);
        //0x34 [15:12]
        adieCR34 = adieCR34 & ~(0xf00);
        adieCR34 = adieCR34 | (VRPI_SEL <<  12);
        //0x38 [25:23]
        adieCR38 = adieCR38 & ~(0x03800000);
        adieCR38 = adieCR38 | (THADC_PGA_GAIN << 23);

        topspi_write(0x34, adieCR34, 0x5);
        topspi_write(0x38, adieCR38, 0x5);

        log_hal_info("adie CR 0x34 : 0x%08lx \r\n", topspi_read(0x34, 0x5));
        log_hal_info("adie CR 0x38 : 0x%08lx \r\n", topspi_read(0x38, 0x5));

    }
    log_hal_info("valid : 0x%02x, VRPI_SEL : 0x%02x, PGA_GAIN : 0X%02x ", valid_bit, VRPI_SEL, THADC_PGA_GAIN);
    //TOP_THADC_SLOPE
    ret = efuse_logical_read_8bits(0xAB, &val);
    if (ret) {
        log_hal_error(READ_EFUSE_FAIL_PATTERN_STR, 0xAB);
        return -1;
    }
    valid_bit = RET_SPECIAL_BITS(val, MASK_PATTERN7, 7);
    RG_THADC_VREF_SEL = RET_SPECIAL_BITS(val, MASK_PATTERN5_6, 5);

    if (valid_bit) {
        adieCR38 = topspi_read(0x38, 0x5);

        //0x38 [27:26]
        adieCR38 = adieCR38 & ~(0x0C000000);
        adieCR38 = adieCR38 | (RG_THADC_VREF_SEL  << 26);
        topspi_write(0x38, adieCR38, 0x5);
        log_hal_info("adie CR 0x38 : 0x%08lx \r\n", topspi_read(0x34, 0x5));
    }
    log_hal_info("valid : 0x%02x, THADC_VREF_SEL : 0x%02x ", valid_bit, RG_THADC_VREF_SEL);
    return 0;

}
int update_formula_value_by_efuse(void)
{
    int ret;
    uint8_t val;
    uint8_t valid_bit = 0;
    uint8_t slope_efuse = 0;
    uint8_t tSensorCAL_efuse = 0;
    uint8_t offset_efuse = 0;


    ret = efuse_logical_read_8bits(0xAB, &val);
    if (ret) {
        log_hal_error(READ_EFUSE_FAIL_PATTERN_STR, 0xAB);
        return -1;
    }
    valid_bit = RET_SPECIAL_BITS(val, MASK_PATTERN7, 7);

    if (valid_bit) {
        slope_efuse  = RET_SPECIAL_BITS(val, MASK_PATTERN4_0, 0);
        slope = slope + slope_efuse;
    }

    ret = efuse_logical_read_8bits(0xAC, &val);
    if (ret) {
        log_hal_error(READ_EFUSE_FAIL_PATTERN_STR, 0xAC);
        return -1;
    }
    valid_bit = RET_SPECIAL_BITS(val, MASK_PATTERN7, 7);

    if (valid_bit) {
        tSensorCAL_efuse  = RET_SPECIAL_BITS(val, MASK_PATTERN6_0, 0);
        compensation = tSensorCAL_efuse;
    }

    ret = efuse_logical_read_8bits(0xAD, &val);
    if (ret) {
        log_hal_error(READ_EFUSE_FAIL_PATTERN_STR, 0xAD);
        return -1;
    }
    valid_bit = RET_SPECIAL_BITS(val, MASK_PATTERN7, 7);

    if (valid_bit) {
        offset_efuse  = RET_SPECIAL_BITS(val, MASK_PATTERN6_0, 0);
        offset = offset + offset_efuse;
    }
    log_hal_info("slope : 0x%02x, compensation : 0x%02x, offset : 0x%02x", slope, compensation, offset);
    return 0;
}

static thermal_status_type ap_thermal_get_cal_val_trigger_mode(adc_value_type adc_type, int *cal)
{
    uint32_t tmp, bk_rs_tmp_cr, u4IterIdx = 0;


    /* Backup configuration */
    bk_rs_tmp_cr = mGetHWEntry32(CONN_THERM_CTL_THERMCR0);

    /* Trigger sampling */
    tmp = mGetHWEntry32(CONN_THERM_CTL_THERMCR0);
    tmp |= (CONN_THERM_CTL_THERMCR0_THERM_CAL_EN_MASK | CONN_THERM_CTL_THERMCR0_THERM_TRIGGER_MASK);
    mSetHWEntry32(CONN_THERM_CTL_THERMCR0, tmp);

    for (u4IterIdx = 0; u4IterIdx < COS_THERMAL_SENSOR_SAMPLING_NUM; u4IterIdx++) {
        hal_gpt_delay_us(1);

        if (mGetHWEntry(CONN_THERM_CTL_THERMCR0_THERM_BUSY) == 0) {
            if (adc_type == MOVING_AVG) {
                *cal = mGetHWEntry(CONN_THERM_CTL_THERMCR0_THERM_CAL_VAL);
            } else {
                *cal = mGetHWEntry(CONN_THERM_CTL_THERMCR0_THERM_RAW_VAL);
            }
            break;
        }
    }

    mSetHWEntry32(CONN_THERM_CTL_THERMCR0, bk_rs_tmp_cr);

    if (u4IterIdx >= COS_THERMAL_SENSOR_SAMPLING_NUM)
        return THER_OPERATION_FAIL;

    return THER_OPERATION_OK;
}

int connsys_thermal_query(int *celsius)
{
    int temp, res;
    bool infra_wakeup = false;
    infra_wakeup = hal_spm_conninfra_is_wakeup();
    if (!infra_wakeup)
        hal_spm_conninfra_wakeup();

    res = get_real_temperature(&temp);

    if (!infra_wakeup)
        hal_spm_conninfra_sleep();

    if (res == THER_OPERATION_OK) {
        *celsius = temp;
        return 0;
    } else {
        return THER_OPERATION_FAIL;
    }
}

int get_real_temperature(int *celsius)
{
    int temperature = 0;
    int raw, res;
    res = ap_thermal_get_cal_val_trigger_mode(MOVING_AVG, &raw);

    if (res == THER_OPERATION_FAIL) {
        log_hal_error("Read thermal raw data error!");
        return -1;
    }
    //log_hal_error("raw data: 0x%08x", raw);

    /* temperature = (y-b)*slope + (offset) */
    temperature = (raw - compensation) * (slope / Default_Slope_denominator) + offset;
    *celsius = temperature;
    //log_hal_error("calculate data: 0x%08x",raw);

    return THER_OPERATION_OK;
}




#endif /* #if defined(HAL_TOP_THERMAL_MODULE_ENABLED) */

