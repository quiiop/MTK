/* Copyright Statement:
 *
 * (C) 2005-2030  MediaTek Inc. All rights reserved.
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ci.h"
#include "ci_cli.h"
#include "hal.h"

#include "hal_gpio_internal.h"
#include "hal_gpt.h"
#include "sound/utils/include/aud_log.h"
#include "sound/include/asound.h"
#include "sound/include/tinypcm.h"
#include "sound/include/tinycompress.h"
#include "sound/driver/include/compress_params.h"
#include "sound/utils/include/afe_reg_rw.h"
#include "sound/utils/include/aud_memory.h"
#include "va.h"
#include "audio_test_utils.h"
#include "hal_psram.h"
#include "audio_task.h"
#include "memory_attribute.h"

ATTR_RWDATA_IN_RAM unsigned int ci_afe_sgen_golden_table_32bits[128] = {
    0x0FE51081, 0x0FE51081, 0x1C446FD3, 0x285E1E49,
    0x285E1E49, 0x3F4A09AA, 0x341446CE, 0x53C7526E,
    0x3F4A09AA, 0x650C65CD, 0x49E3C3B3, 0x726F5D86,
    0x53C7526E, 0x7B6C874D, 0x5CDC5481, 0x7FAB7471,
    0x650C65CD, 0x7F025FD4, 0x6C4356A3, 0x7977C8DF,
    0x726F5D86, 0x6F42338F, 0x77814308, 0x60C61029,
    0x7B6C874D, 0x4E91DF1A, 0x7E2780DC, 0x3958B70F,
    0x7FAB7471, 0x21EB6312, 0x7FF4A595, 0x09305C79,
    0x7F025FD4, 0xF01AEF7F, 0x7CD6F87E, 0xD7A1E1B7,
    0x7977C8DF, 0xC0B5F656, 0x74ED2115, 0xAC38AD92,
    0x6F42338F, 0x9AF39A33, 0x6884F973, 0x8D90A27A,
    0x60C61029, 0x849378B3, 0x58189067, 0x80548B8F,
    0x4E91DF1A, 0x80FDA02C, 0x444978AA, 0x86883721,
    0x3958B70F, 0x90BDCC71, 0x2DDA9352, 0x9F39EFD7,
    0x21EB6312, 0xB16E20E6, 0x15A8929F, 0xC6A748F1,
    0x09305C79, 0xDE149CEE, 0xFCA17EC4, 0xF6CFA387,
    0xF01AEF7F, 0x0FE51081, 0xE3BB902D, 0x285E1E49,
    0xD7A1E1B7, 0x3F4A09AA, 0xCBEBB932, 0x53C7526E,
    0xC0B5F656, 0x650C65CD, 0xB61C3C4D, 0x726F5D86,
    0xAC38AD92, 0x7B6C874D, 0xA323AB7F, 0x7FAB7471,
    0x9AF39A33, 0x7F025FD4, 0x93BCA95D, 0x7977C8DF,
    0x8D90A27A, 0x6F42338F, 0x887EBCF8, 0x60C61029,
    0x849378B3, 0x4E91DF1A, 0x81D87F24, 0x3958B70F,
    0x80548B8F, 0x21EB6312, 0x800B5A6B, 0x09305C79,
    0x80FDA02C, 0xF01AEF7F, 0x83290782, 0xD7A1E1B7,
    0x86883721, 0xC0B5F656, 0x8B12DEEB, 0xAC38AD92,
    0x90BDCC71, 0x9AF39A33, 0x977B068D, 0x8D90A27A,
    0x9F39EFD7, 0x849378B3, 0xA7E76F99, 0x80548B8F,
    0xB16E20E6, 0x80FDA02C, 0xBBB68756, 0x86883721,
    0xC6A748F1, 0x90BDCC71, 0xD2256CAE, 0x9F39EFD7,
    0xDE149CEE, 0xB16E20E6, 0xEA576D61, 0xC6A748F1,
    0xF6CFA387, 0xDE149CEE, 0x035E813C, 0xF6CFA387,
};

ATTR_RWDATA_IN_RAM unsigned int ci_afe_sgen_golden_table_16bits[64] = {
    0x0FE50FE5, 0x285E1C44, 0x3F4A285E, 0x53C73414,
    0x650C3F4A, 0x726F49E3, 0x7B6C53C7, 0x7FAB5CDC,
    0x7F02650C, 0x79776C43, 0x6F42726F, 0x60C67781,
    0x4E917B6C, 0x39587E27, 0x21EB7FAB, 0x09307FF4,
    0xF01A7F02, 0xD7A17CD6, 0xC0B57977, 0xAC3874ED,
    0x9AF36F42, 0x8D906884, 0x849360C6, 0x80545818,
    0x80FD4E91, 0x86884449, 0x90BD3958, 0x9F392DDA,
    0xB16E21EB, 0xC6A715A8, 0xDE140930, 0xF6CFFCA1,
    0x0FE5F01A, 0x285EE3BB, 0x3F4AD7A1, 0x53C7CBEB,
    0x650CC0B5, 0x726FB61C, 0x7B6CAC38, 0x7FABA323,
    0x7F029AF3, 0x797793BC, 0x6F428D90, 0x60C6887E,
    0x4E918493, 0x395881D8, 0x21EB8054, 0x0930800B,
    0xF01A80FD, 0xD7A18329, 0xC0B58688, 0xAC388B12,
    0x9AF390BD, 0x8D90977B, 0x84939F39, 0x8054A7E7,
    0x80FDB16E, 0x8688BBB6, 0x90BDC6A7, 0x9F39D225,
    0xB16EDE14, 0xC6A7EA57, 0xDE14F6CF, 0xF6CF035E,
};

ci_status_t ci_etdmout2_sample(void)
{
    int channel_num = 2;
    int bitdepth = 16;
    int sample_rate = 16000;
    int period_size = 960;
    int period_count = 4;
    int time_len = 4;

    sound_t *w_snd;
    int ret, index;
    const void *golden_src;
    int golden_size;
    struct msd_hw_params params;
    ci_status_t flag = CI_PASS;

    hal_pinmux_set_function(HAL_GPIO_11, MT7933_PIN_11_FUNC_I2SO_DAT0);
    hal_pinmux_set_function(HAL_GPIO_13, MT7933_PIN_13_FUNC_I2SO_BCK);
    hal_pinmux_set_function(HAL_GPIO_14, MT7933_PIN_14_FUNC_I2SO_LRCK);
    hal_pinmux_set_function(HAL_GPIO_15, MT7933_PIN_15_FUNC_I2SO_MCK);

    switch (bitdepth) {
        case 32:
            golden_src = ci_afe_sgen_golden_table_32bits;
            golden_size = 512;
            break;

        case 16:
            golden_src = ci_afe_sgen_golden_table_16bits;
            golden_size = 256;
            break;

        default:
            printf("bitdepth error: %d", bitdepth);
            return CI_FAIL;
    }

    params.format = bitdepth == 16 ? MSD_PCM_FMT_S16_LE : MSD_PCM_FMT_S32_LE;
    params.channels = channel_num;
    params.period_count = period_count;
    params.period_size = period_size;
    params.rate = sample_rate;
    int bytes_per_frame = bitdepth * params.channels / 8;
    int data_size = bytes_per_frame * params.period_size * params.period_count / golden_size * golden_size;

    void *data_src = malloc(data_size);
    if (!data_src) {
        printf("%s: memory error\n", __FUNCTION__);
        return CI_FAIL;
    }

    memset(data_src, 0, data_size);

    for (index = 0; index < data_size / golden_size; index++) {
        memcpy(data_src + index * golden_size, golden_src, golden_size);
    }

    connect_route("track0", "ETDM2_OUT_BE", 1, CONNECT_FE_BE);
    connect_route("I_22", "O_04", 1, CONNECT_IO_PORT);
    connect_route("I_23", "O_05", 1, CONNECT_IO_PORT);

    ret = snd_pcm_open(&w_snd, "track0", 0, 0);
    if (ret) {
        flag += 1;
        goto exit0;
    }

    ret = snd_pcm_hw_params(w_snd, &params);
    if (ret) {
        flag += 1;
        goto exit1;
    }

    ret = snd_pcm_prepare(w_snd);
    if (ret) {
        flag += 1;
        goto exit1;
    }

    int total_frames = time_len * params.rate;
    while (total_frames > 0) {
        ret = snd_pcm_write(w_snd, data_src, data_size / bytes_per_frame);
        if (ret != data_size / bytes_per_frame) {
            printf("ret: %d", ret);
        }
        total_frames = total_frames - ret;
    }

    ret = snd_pcm_drop(w_snd);
    if (ret) {
        flag += 1;
        goto exit1;
    }
    ret = snd_pcm_hw_free(w_snd);
    if (ret) {
        flag += 1;
        goto exit1;
    }

exit1:
    snd_pcm_close(w_snd);

exit0:
    free(data_src);
    connect_route("track0", "ETDM2_OUT_BE", 0, CONNECT_FE_BE);
    connect_route("I_22", "O_04", 0, CONNECT_IO_PORT);
    connect_route("I_23", "O_05", 0, CONNECT_IO_PORT);

    return (flag ? CI_FAIL : CI_PASS);
}

ci_status_t ci_i2s_sample_main(unsigned int portnum)
{
    struct test_entry test_entry_list[] = {
        {"Sample Code: i2s sample", ci_etdmout2_sample},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}
