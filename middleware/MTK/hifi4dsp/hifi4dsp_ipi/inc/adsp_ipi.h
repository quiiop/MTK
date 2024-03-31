/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#ifndef ADSP_IPI_H
#define ADSP_IPI_H
/**
 *@addtogroup Adsp_ipi
 *@{
 * @brief This section introduces the adsp_ipi APIs including terms and acronyms, supported features, details on how to use the adsp_ipi function groups.
 *
 * adsp_ipi is a commucation between adsp & ap
 *
 * @section ADSP_IPI_Features_Chapter Supported features
 * - \b adsp_ipi_send \b
 *   adsp_ipi_status adsp_ipi_send(uint32_t id, void *buf, uint32_t len, uint32_t wait, enum ipi_dir dir);
 *   Transfer specific cmd id and buffer to opposite end
 *
 * - \b adsp_ipi_registration \b
 *   adsp_ipi_status adsp_ipi_registration(uint32_t id, ipi_handler_t handler, const char* name);
 *   Register callback function for  specific cmd id. The callback function will be called after corresponding cmd id received.
 *
 * @section Add a new IPI cmd IPI_NEW_CMD, send by AP, handle by ADSP
 *
 * Add IPI_NEW_CMD in "enum ipi_id" in adsp_ipi.h[AP side & DSP side] (IPI cmd must defined both in AP side and ADSP side and keep sync)
 * Define a ipi_handler_t function "new_cmd_handler” in ADSP side, "typedef int (*ipi_handler_t)(int id, void* data, unsigned int len)"
 * Register ipi cmd handler in ADSP module init function, adsp_ipi_registration(IPI_NEW_CMD, new_cmd_handler, "new cmd handler");
 * When AP want to send ipi msg to ADSP, call adsp_ipi_send(IPI_NEW_CMD, buffer, len, 1, 0) in AP side.
 * ADSP IPI module will transfer the msg from AP to ADSP, and callback IPI_NEW_CMD handler registered in ADSP side,  new_cmd_handler
 * -- buffer len is limited to 272 bytes
 * -- when new_cmd_handler is callbacked, the content of parameter "data/len" is same as the content in AP side
 *
 * @section Add a new IPI cmd IPI_NEW_CMD, send by ADSP, handle by AP
 * Add IPI_NEW_CMD in "enum ipi_id" in adsp_ipi.h[AP side & DSP side] (IPI cmd must defined both in AP side and ADSP side and keep sync)
 * Define a ipi_handler_t function "new_cmd_handler” in AP side, "typedef int (*ipi_handler_t)(int id, void* data, unsigned int len)"
 * Register ipi cmd handler in AP module init function, adsp_ipi_registration(IPI_NEW_CMD, new_cmd_handler, "new cmd handler");
 * When ADSP want to send ipi msg to AP, call adsp_ipi_send(IPI_NEW_CMD, buffer, len, 1, 0) in ADSP side.
 * ADSP IPI module will transfer the msg from ADSP to AP, and callback IPI_NEW_CMD handler registered in AP side,  new_cmd_handler

 * @note    The cmd id definition must keep consistent between AP and ADSP.
 *          Callback function is called in msg queue one-by-one, please keep callback function simply and short.
 */


/****************************************************************************
 *
 * Constants.
 *
 ****************************************************************************/

#include <adsp_ipi_common.h>

#define HOST_TARGET_ID 0x85188518

/* test mode */
//#define IPI_TEST  (1)
//#define FAKE_HOST_IPC_UT 1

/* please add IPI_TEST_UT in ipi_id if enable FAKE_HOST_IPC_UT_TEST */
//#define FAKE_HOST_IPC_UT_TEST 1

/* NOTICE:
 * Please keep ipi_id define is synced with host driver !
 */
enum ipi_id {
    IPI_WDT = 0,
    IPI_TEST1,
    IPI_AUDIO,
    IPI_SPEECH_ADSP_TO_AP,
    IPI_SPEECH_AP_TO_ADSP,
    /* NOTICE: please add all ut ipi id in FAKE_HOST_IPC_UT */
#ifdef FAKE_HOST_IPC_UT
    IPI_AUDIO_FAKE_HOST,
    IPI_TEST_UT,
    NR_IPI,
#else /* #ifdef FAKE_HOST_IPC_UT */
    NR_IPI,
#endif /* #ifdef FAKE_HOST_IPC_UT */
};

/*****************************************************************************
* Functions
*****************************************************************************/

/**
 * @brief     ADSP IPI init function.
 * @para      none
 * @return    none.
 */
void adsp_ipi_init(void);
/**
 * @brief     ADSP IPI registration function.
 * @para
 *            id:        which ipi event you want to send to
 *            handler:   which callback you want to call after send
 *            name:      the name of ipi event
 *
 * @return    DONE on success or a negative error.
 */
ipi_status adsp_ipi_registration(uint32_t id, ipi_handler_t handler, const char *name);
/**
 * @brief     ADSP IPI send function.
 * @para
 *            id:        which ipi event you want to send to
 *            buf:       the buffer you want to send
 *            len:       the length of ipi buffer
 *            wait:      not used
 *            dir:       not used
 *
 * @return    DONE on success or a negative error.
 */
ipi_status adsp_ipi_send(uint32_t id, void *buf, uint32_t len, uint32_t wait, enum ipi_dir dir);

/**
* @}
*/


#endif /* #ifndef ADSP_IPI_H */
