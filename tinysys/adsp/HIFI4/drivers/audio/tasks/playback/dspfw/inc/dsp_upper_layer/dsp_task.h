/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2019. All rights reserved.
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

 
#ifndef _DSP_TASK_H_
#define _DSP_TASK_H_

#include "audio_types.h"
#include "FreeRTOS.h"
#include "task.h"


/******************************************************************************
 * External Global Variables
 ******************************************************************************/


extern TaskHandle_t  pDTM_TaskHandler;
extern TaskHandle_t  pDAV_TaskHandler;
extern TaskHandle_t  pDPR_TaskHandler;
extern TaskHandle_t  pDHP_TaskHandler;

#define	DAV_TASK_ID pDAV_TaskHandler
#define	DPR_TASK_ID pDPR_TaskHandler
#define	DHP_TASK_ID pDHP_TaskHandler
#define	DSP_TASK_ID pDTM_TaskHandler
#define	NULL_TASK_ID NULL





/******************************************************************************
 * Inline Functions
 ******************************************************************************/
#define DSP_OFFSET_OF(type,member) ((SIZE)&(((type *)0)->member))
	
	/**
	 * @brief cast a member of a structure out to the containing structure
	 * @param ptr			  the pointer to the member.
	 * @param type			  the type of the container struct this is embedded in.
	 * @param member		  the name of the member within the struct.
	 */
#define DSP_CONTAINER_OF(ptr,type,member) ((type *)((U8 *)(ptr) - DSP_OFFSET_OF(type,member)))

#define DSP_UPDATE_COMMAND_FEATURE_PARA_SEQUENCE_AUTODETECT  0xFFFFFFFF

/******************************************************************************
* Interrupt handler related
******************************************************************************/
#define INTR_ID_SRC_IN                       (0)
#define INTR_ID_IDFE_AU                      (1)
#define INTR_ID_ODFE_AU                      (2)
#define INTR_ID_ODFE_VP                      (3)
#define INTR_ID_SPDIF                        (4)

extern VOID INTR_RegisterHandler( U8 IntrId, VOID* IntrHdlr, TaskHandle_t  TaskId);
extern VOID INTR_CancelHandler(U8 IntrId);

typedef VOID (*CRITICALFUN)(VOID* para);
extern	VOID PL_CRITICAL(CRITICALFUN pFun , VOID* para);


#endif /* _DSP_TASK_H_ */

