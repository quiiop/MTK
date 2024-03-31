/*
 * mt7933-adsp-utils.c  --  Mediatek 7933 adsp utility
 *
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Mengge Wang <mengge.wang@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "mt7933-adsp-utils.h"
#include "mt7933-afe-common.h"
#include "audio_shared_info.h"
#include "aud_log.h"
#include "errno.h"
#include <string.h>
#include "hal_nvic.h"
#include "hal_sleep_manager_internal.h"

unsigned int bypass_ipi_send_to_dsp = 0;

int mt7933_adsp_get_scene_by_dai_id(int id)
{
    switch (id) {
        case MT7933_ADSP_FE_LOCAL_RECORD:
        case MT7933_ADSP_FE_AP_RECORD:
        case MT7933_ADSP_BE_UL2:
            return TASK_SCENE_RECORD;
        case MT7933_ADSP_FE_HOSTLESS_VA:
        case MT7933_ADSP_FE_VA:
        case MT7933_ADSP_BE_UL9:
            return TASK_SCENE_VA;
        default:
            aud_error("Not a get scene support ID %d.\n", id);
            break;
    }

    return -1;
}

int mt7933_adsp_send_ipi_cmd(struct ipi_msg_t *p_msg,
                             uint8_t task_scene,
                             uint8_t target,
                             uint8_t data_type,
                             uint8_t ack_type,
                             uint16_t msg_id,
                             uint32_t param1,
                             uint32_t param2,
                             char *payload)
{
    struct ipi_msg_t ipi_msg;
    struct ipi_msg_t *msg;
    int ret = 0;
    aud_msg("[%s]\n", __func__);

    if (bypass_ipi_send_to_dsp) {
        aud_msg("%s [Bypass send to DSP] task scene(%d), msg id(%d), msg name(%s)\n",
                __func__, task_scene, msg_id, msg_string[msg_id]);
        return 0;
    }

    memset((void *)&ipi_msg, 0, sizeof(struct ipi_msg_t));

    if (p_msg)
        msg = p_msg;
    else
        msg = &ipi_msg;

    ret = audio_send_ipi_msg(msg, task_scene, target, data_type,
                             ack_type, msg_id, param1, param2,
                             (char *)payload);
    if (ret != 0)
        aud_msg("%s audio_send_ipi_msg (%d-%d-%d-%d) fail %d\n",
                __func__, task_scene, data_type,
                ack_type, msg_id, ret);

    return ret;
}

int mt7933_adsp_dai_id_pack(int dai_id)
{
    //return dai_id;

    int id = -EINVAL;
    // TODO: change id
    switch (dai_id) {
        case MT7933_ADSP_FE_LOCAL_RECORD:
            id = DAI_PACK_ID(MT7933_ADSP_FE_LOCAL_RECORD,
                             DAI_MIC_RECORD_TYPE, DAI_NON_HOSTLESS);
            break;
        case MT7933_ADSP_FE_AP_RECORD:
            id = DAI_PACK_ID(MT7933_ADSP_FE_AP_RECORD,
                             DAI_MIC_RECORD_TYPE, DAI_NON_HOSTLESS);
            break;
        case MT7933_ADSP_FE_HOSTLESS_VA:
            id = DAI_PACK_ID(MT7933_ADSP_FE_VA,
                             DAI_VA_RECORD_TYPE, DAI_HOSTLESS);
            break;
        case MT7933_ADSP_FE_VA:
            id = DAI_PACK_ID(MT7933_ADSP_FE_VA,
                             DAI_VA_RECORD_TYPE, DAI_NON_HOSTLESS);
            break;
        default:
            aud_error("Not a pack support ID %d.\n", dai_id);
            break;
    }
    return id;
}

int mt7933_adsp_get_afe_memif_id(int dai_id)
{
    int memif_id = 0;

    switch (dai_id) {
        case MT7933_ADSP_BE_UL9:
            memif_id = MT7933_AFE_MEMIF_UL9;
            break;
        case MT7933_ADSP_BE_UL2:
            memif_id = MT7933_AFE_MEMIF_UL2;
            break;
        case MT7933_ADSP_BE_DL2:
            memif_id = MT7933_AFE_MEMIF_DL2;
            break;
        case MT7933_ADSP_BE_DL3:
            memif_id = MT7933_AFE_MEMIF_DL3;
            break;
        case MT7933_ADSP_BE_DLM:
            memif_id = MT7933_AFE_MEMIF_DLM;
            break;
        default:
            memif_id = -EINVAL;
            break;
    }
    return memif_id;
}

bool mt7933_adsp_need_ul_dma_copy(int id)
{
    switch (id) {
        case MT7933_ADSP_FE_AP_RECORD:
            return true;
        default:
            break;
    }

    return false;
}

void set_execption_happen_flag(void)
{
    bypass_ipi_send_to_dsp = 1;
}

void clear_execption_happen_flag(void)
{
    bypass_ipi_send_to_dsp = 0;
}

/************************************************************************

            DSP to CM33, CM33 to DSP irq control

************************************************************************/

#define RG_DSPIRQS_CM33_TO_DSP_INT_TRIG      (0x41002000 + 0x104)
#define RG_DSPIRQS_DSP_TO_CM33_INT_STS       (0x41002000 + 0x118)

#define WriteReg32(addr,data)                ((*(volatile unsigned int *)(addr)) = (unsigned int)(data))
#define ReadReg32(addr)                      (*(volatile unsigned int *)(addr))
#define WriteReg32Mask(addr, val, msk) \
    WriteReg32((addr), ((ReadReg32(addr) & (~(msk))) | ((val) & (msk))))


#define TRIGGER_INT_TO_DSP(BIT) \
    do { \
        WriteReg32Mask(RG_DSPIRQS_CM33_TO_DSP_INT_TRIG, 0x1 << BIT, 0x1 << BIT); \
    } while (0)

#define CLEAR_INT_FROM_DSP(BIT) \
    do { \
        WriteReg32Mask(RG_DSPIRQS_DSP_TO_CM33_INT_STS, 0x1 << BIT, 0x1 << BIT); \
    } while (0)

enum {
    INT_SLEEP_PSRAM = 0,
    INT_WAKEUP_PSRAM = 1,
    INT_AP_SUSPEND = 2,
    INT_AP_RESUME = 3,
    INT_WAKEUP_PSRAM2 = 4,
    INT_NUM,
};

struct dsp2ap_irq_data {
    void (*irq_callback)(void);
};

/* Notify DSP that cm33 is going suspend */
int ap2dsp_suspend_notify(void)
{
    TRIGGER_INT_TO_DSP(INT_AP_SUSPEND);
    return 0;
}

/* Notify DSP that cm33 is going resume */
int ap2dsp_resume_notify(void)
{
    TRIGGER_INT_TO_DSP(INT_AP_RESUME);
    return 0;
}

static void irq_callback_ack_sleep_psram(void)
{
    hal_sleep_manager_unlock_sleep(SLEEP_LOCK_CM33);
    TRIGGER_INT_TO_DSP(INT_SLEEP_PSRAM);
    //  aud_msg("irq_callback_ack_sleep_psram");
}

static void irq_callback_ack_wakeup_psram(void)
{
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_CM33);
    TRIGGER_INT_TO_DSP(INT_WAKEUP_PSRAM);
    //  aud_msg("irq_callback_ack_wakeup_psram");
}

static void irq_callback_notify_ap_suspend(void)
{
    aud_msg("irq_callback_notify_ap_auspend");
}

static void irq_callback_notify_ap_resume(void)
{
    aud_msg("irq_callback_notify_ap_resume");
}

static void irq_callback_wakeup_psram(void)
{
    hal_sleep_manager_lock_sleep(SLEEP_LOCK_PSRAM);
    //  aud_msg("irq_callback_wakeup_psram");
}

static struct dsp2ap_irq_data irq_data[INT_NUM] = {
    [INT_SLEEP_PSRAM] = {
        .irq_callback = irq_callback_ack_sleep_psram,
    },
    [INT_WAKEUP_PSRAM] = {
        .irq_callback = irq_callback_ack_wakeup_psram,
    },
    [INT_AP_SUSPEND] = {
        .irq_callback = irq_callback_notify_ap_suspend,
    },
    [INT_AP_RESUME] = {
        .irq_callback = irq_callback_notify_ap_resume,
    },
    [INT_WAKEUP_PSRAM2] = {
        .irq_callback = irq_callback_wakeup_psram,
    },
};

static void dsp2ap_irq_handler(hal_nvic_irq_t irq)
{
    unsigned int status;
    int i;

    status = ReadReg32(RG_DSPIRQS_DSP_TO_CM33_INT_STS);
    for (i = 0; i < INT_NUM; i++) {
        if (((status & 1U << (i)) != 0) &&
            (irq_data[i].irq_callback != NULL)) {
            irq_data[i].irq_callback();
        }
    }

    /* clear irq status */
    DRV_WriteReg32(RG_DSPIRQS_DSP_TO_CM33_INT_STS, status);
}

void dsp2ap_irq_init(void)
{
    int ret = 0;

    /* register irq*/
    ret = hal_nvic_register_isr_handler(DSP_TO_CM33_IRQn, dsp2ap_irq_handler);
    if (ret != 0) {
        aud_error("could not request_irq");
        return;
    }
    /* enable irq */
    ret = hal_nvic_enable_irq(DSP_TO_CM33_IRQn);
    if (ret != 0) {
        aud_error("could not enable_irq");
        return;
    }

}

void dsp2ap_irq_uninit(void)
{
    int ret = 0;

    /* disable irq */
    ret = hal_nvic_disable_irq(DSP_TO_CM33_IRQn);
    if (ret != 0) {
        aud_error("could not enable_irq");
        return;
    }
}

