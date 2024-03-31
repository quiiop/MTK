/*
 * MediaTek Inc. (C) 2018. All rights reserved.
 *
 * Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */
#include "preproc_process.h"
#include "chdemux_process.h"
#ifdef DUMP_TO_HOST_SUPPORT
#include "audio_dump_helper.h"
#endif
#include "mt_printf.h"
#include "systimer.h"
#include "audio_task.h"
#include "mtk_heap.h"

#ifdef VA_MTK_PREPROC_SUPPORT
#include "mtk_effp_adaptor.h"
#endif

#ifdef VA_DUMMY_AEC_SUPPORT
#include "mtk_aec_dummy_adaptor.h"
#endif

#ifdef CFG_VA_PREPROC_MCPS_CALC
DEFINE_MCPS_CALC(preproc_mcps);
#endif

static struct algo_ops algo_adaptor;

NORMAL_SECTION_FUNC static int preproc_init(struct va_proc_obj *obj, struct va_pcm_format in, struct va_pcm_format *out, int frames)
{
    struct preproc_private *priv = (struct preproc_private *)(obj->priv);
    struct preproc_config *config = (struct preproc_config *)(&(priv->config));

    if (out == NULL)
        return -22;

    config->rate = in.rate;
    config->bitwidth = in.bitwidth;
    config->proc_frames = frames;
    config->in_chnum = in.chnum;
    config->out_chnum = CFG_VA_AEC_OUT_CH_NUM;

    if (config->bitwidth != CFG_VA_AEC_BITWIDTH) {
        config->bitwidth = CFG_VA_AEC_BITWIDTH;
        /* Need temp buffer and format convert */
        struct chdemux_out out_config;
        uint32_t tmp_buf_size;
        int mem_type = get_adsp_heap_type(ADSP_MEM_LP_CACHE);

        chdemux_proc_create(&(priv->fmt_cvt));
        out_config.bitwidth = config->bitwidth;
        out_config.out_chnum = config->in_chnum;
        out_config.out_chidx[0] = 0;
        out_config.out_chidx[1] = 1;
        out_config.out_chidx[2] = 2;
        out_config.out_chidx[3] = 3;
        priv->fmt_cvt->set_params(priv->fmt_cvt, CMD_SET_CHDEMUX_OUT, &out_config);
        priv->fmt_cvt->init(priv->fmt_cvt, in, NULL, frames);
        tmp_buf_size = frames * out_config.out_chnum * out_config.bitwidth / 8;
        priv->tmp_buf = (char *)(MTK_pvPortMalloc(tmp_buf_size, mem_type));
        configASSERT(priv->tmp_buf != NULL);
        PRINTF_D("do preproc format convert\n");
    }

    out->rate = config->rate;
    out->bitwidth = config->bitwidth;
    out->chnum = config->out_chnum;

    if (algo_adaptor.init != NULL)
		algo_adaptor.init(obj, in, out, frames);

    obj->state = PROC_STAT_INIT;
    PRINTF_D("%s [time:%llu] rate(%d), in_bitwidth(%d), out_bitwidth(%d), in_chnum(%d), out_chnum(%d)\n",
		__func__, read_systimer_stamp_ns(), config->rate, config->bitwidth, out->bitwidth, config->in_chnum, config->out_chnum);

#ifdef CFG_VA_PREPROC_MCPS_CALC
    mcps_calc_init(&preproc_mcps);
#endif

    return 0;
}

NORMAL_SECTION_FUNC static int preproc_uninit(struct va_proc_obj *obj)
{
    struct preproc_private *priv = (struct preproc_private *)(obj->priv);
    PRINTF_D("%s [time:%llu]\n", __func__, read_systimer_stamp_ns());

    if (obj->state != PROC_STAT_INIT)
        return -1;

    if (priv->fmt_cvt != NULL) {
        chdemux_proc_destroy(priv->fmt_cvt);
        priv->fmt_cvt = NULL;
    }

    if (priv->tmp_buf) {
        MTK_vPortFree(priv->tmp_buf);
        priv->tmp_buf = NULL;
    }

    if (algo_adaptor.uninit != NULL)
		algo_adaptor.uninit(obj);

    obj->state = PROC_STAT_UNINIT;
    return 0;
}

static int preproc_set_params(struct va_proc_obj *obj, int cmd, void *data)
{
    switch(cmd){
    case CMD_SET_AEC_CH_CONFIG:
        break;
    case CMD_SET_AEC_WW_OK:
        break;
    case CMD_SET_AEC_WW_START:
        break;
    default:
        break;
    }

    if (algo_adaptor.set_params != NULL)
		algo_adaptor.set_params(obj, cmd, data);

    return 0;
}

static int preproc_get_params(struct va_proc_obj *obj, int cmd, void *data)
{
    switch(cmd){
    case CMD_GET_BEAMFORMING_RESULT:
        break;
    default:
        break;
    }

    if (algo_adaptor.get_params != NULL)
		algo_adaptor.get_params(obj, cmd, data);

    return 0;
}

static int preproc_reset(struct va_proc_obj *obj)
{
    PRINTF_D("%s [time:%llu]\n", __func__, read_systimer_stamp_ns());
    if (obj->state != PROC_STAT_INIT)
        return -1;

    if (algo_adaptor.reset != NULL)
		algo_adaptor.reset(obj);

    return 0;
}

static int preproc_process(struct va_proc_obj *obj, char *inbuf, char *outbuf, int frames)
{
    struct preproc_private *priv = (struct preproc_private *)(obj->priv);
    int ret = 0;
    char *preproc_proc_buf = inbuf;

    if (obj->state != PROC_STAT_INIT)
        return -1;

    if (priv->fmt_cvt != NULL) {
        priv->fmt_cvt->process(priv->fmt_cvt, inbuf, priv->tmp_buf, frames);
        preproc_proc_buf = priv->tmp_buf;
    }

#ifdef DUMP_TO_HOST_SUPPORT
    audio_dump_helper_write(DSP_DUMP2, preproc_proc_buf, frames);
#endif

#ifdef CFG_VA_PREPROC_MCPS_CALC
    mcps_calc_before_proc(&preproc_mcps);
#endif

    if (algo_adaptor.process != NULL) {
        ret = algo_adaptor.process(obj, preproc_proc_buf, outbuf, frames);
    }

#ifdef CFG_VA_PREPROC_MCPS_CALC
    mcps_calc_after_proc(&preproc_mcps);
#endif

#ifdef DUMP_TO_HOST_SUPPORT
    audio_dump_helper_write(DSP_DUMP3, outbuf, frames);
#endif


    return ret;
}

NORMAL_SECTION_FUNC int preproc_proc_create(struct va_proc_obj **obj)
{
    struct va_proc_obj *preproc = NULL;
    int mem_type = get_adsp_heap_type(ADSP_MEM_LP_CACHE);

    preproc =
        (struct va_proc_obj *)(MTK_pvPortMalloc(sizeof(struct va_proc_obj), mem_type));
    configASSERT(preproc != NULL);
    memset(preproc, 0, sizeof(struct va_proc_obj));
    preproc->type = VA_PROC_PREPROC;
    preproc->priv =
        (struct preproc_private *)(MTK_pvPortMalloc(sizeof(struct preproc_private), mem_type));
    configASSERT(preproc->priv != NULL);
    memset(preproc->priv, 0, sizeof(struct preproc_private));
    preproc->init = preproc_init;
    preproc->uninit = preproc_uninit;
    preproc->get_params = preproc_get_params;
    preproc->set_params = preproc_set_params;
    preproc->reset = preproc_reset;
    preproc->process = preproc_process;
    preproc->state = PROC_STAT_UNINIT;

    *obj = preproc;

#ifdef VA_MTK_PREPROC_SUPPORT
    mtk_effp_adaptor_register(&algo_adaptor);
#endif

#ifdef VA_DUMMY_AEC_SUPPORT
    mtk_aec_dummy_adaptor_register(&algo_adaptor);
#endif

    PRINTF_D("%s [time:%llu] preproc create success\n", __func__, read_systimer_stamp_ns());
    return 0;
}

NORMAL_SECTION_FUNC int preproc_proc_destroy(struct va_proc_obj *obj)
{
    if (obj->priv)
        MTK_vPortFree(obj->priv);
    MTK_vPortFree(obj);
    memset(&algo_adaptor, 0, sizeof(algo_adaptor));
    PRINTF_D("%s [time:%llu] preproc destroy success\n", __func__, read_systimer_stamp_ns());
    return 0;
}
