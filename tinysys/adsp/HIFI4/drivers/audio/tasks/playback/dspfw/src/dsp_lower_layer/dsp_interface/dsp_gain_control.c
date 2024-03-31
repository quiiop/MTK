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


#include "audio_types.h"
#include "dsp_audio_ctrl.h"
#include "stream_audio_driver.h"
#ifdef HAL_AUDIO_READY
#include "hal_audio.h"
#endif
#include "audio_nvdm_common.h"
#include "dsp_temp.h"

#ifdef AIR_SOFTWARE_GAIN_ENABLE
#include "stream_n9sco.h"
#endif

/******************************************************************************
 * Definitions
 ******************************************************************************/
#define Q11_TABLE_SIZE (sizeof(DSPGAIN_Q11)/sizeof(DSPGAIN_Q11[0]))
extern void hal_audio_set_stream_in_volume_for_multiple_microphone(uint16_t volume_index0, uint16_t volume_index1, hal_audio_input_gain_select_t gain_select);



/******************************************************************************
 * Tables
 ******************************************************************************/
const S16 DSPGAIN_Q11[124] =
{
    0x0,  //for mute
    0x2,  //-60.0dB
    0x3,  //-57.0dB
    0x4,  //-54.0dB
    0x5,  //-52.0dB
    0x7,  //-49.0dB
    0x8,  //-48.0dB
    0x9,  //-47.0dB
    0xA,  //-46.0dB
    0xC,  //-45.0dB
    0xD,  //-44.0dB
    0xE,  //-43.0dB
    0x10,  //-42.0dB
    0x12,  //-41.0dB
    0x14,  //-40.0dB
    0x17,  //-39.0dB
    0x1A,  //-38.0dB
    0x1D,  //-37.0dB
    0x1F,  //-36.5dB
    0x20,  //-36.0dB
    0x22,  //-35.5dB
    0x24,  //-35.0dB
    0x27,  //-34.5dB
    0x29,  //-34.0dB
    0x2B,  //-33.5dB
    0x2E,  //-33.0dB
    0x31,  //-32.5dB
    0x33,  //-32.0dB
    0x36,  //-31.5dB
    0x3A,  //-31.0dB
    0x3D,  //-30.5dB
    0x41,  //-30.0dB
    0x45,  //-29.5dB
    0x49,  //-29.0dB
    0x4D,  //-28.5dB
    0x52,  //-28.0dB
    0x56,  //-27.5dB
    0x5B,  //-27.0dB
    0x61,  //-26.5dB
    0x67,  //-26.0dB
    0x6D,  //-25.5dB
    0x73,  //-25.0dB
    0x7A,  //-24.5dB
    0x81,  //-24.0dB
    0x89,  //-23.5dB
    0x91,  //-23.0dB
    0x9A,  //-22.5dB
    0xA3,  //-22.0dB
    0xAC,  //-21.5dB
    0xB7,  //-21.0dB
    0xC1,  //-20.5dB
    0xCD,  //-20.0dB
    0xD9,  //-19.5dB
    0xE6,  //-19.0dB
    0xF3,  //-18.5dB
    0x102,  //-18.0dB
    0x111,  //-17.5dB
    0x121,  //-17.0dB
    0x132,  //-16.5dB
    0x145,  //-16.0dB
    0x158,  //-15.5dB
    0x16C,  //-15.0dB
    0x182,  //-14.5dB
    0x199,  //-14.0dB
    0x1B1,  //-13.5dB
    0x1CA,  //-13.0dB
    0x1E6,  //-12.5dB
    0x202,  //-12.0dB
    0x221,  //-11.5dB
    0x241,  //-11.0dB
    0x263,  //-10.5dB
    0x288,  //-10.0dB
    0x2AE,  //-9.5dB
    0x2D7,  //-9.0dB
    0x302,  //-8.5dB
    0x32F,  //-8.0dB
    0x360,  //-7.5dB
    0x393,  //-7.0dB
    0x3C9,  //-6.5dB
    0x402,  //-6.0dB
    0x43F,  //-5.5dB
    0x480,  //-5.0dB
    0x4C4,  //-4.5dB
    0x50C,  //-4.0dB
    0x559,  //-3.5dB
    0x5AA,  //-3.0dB
    0x600,  //-2.5dB
    0x65B,  //-2.0dB
    0x6BB,  //-1.5dB
    0x721,  //-1.0dB
    0x78D,  //-0.5dB
    0x800,  //0.0dB
    0x879,  //0.5dB
    0x8FA,  //1.0dB
    0x982,  //1.5dB
    0xA12,  //2.0dB
    0xAAB,  //2.5dB
    0xB4D,  //3.0dB
    0xBF8,  //3.5dB
    0xCAE,  //4.0dB
    0xD6E,  //4.5dB
    0xE3A,  //5.0dB
    0xF12,  //5.5dB
    0xFF6,  //6.0dB
    0x10E8,  //6.5dB
    0x11E9,  //7.0dB
    0x12F9,  //7.5dB
    0x1418,  //8.0dB
    0x1549,  //8.5dB
    0x168C,  //9.0dB
    0x17E2,  //9.5dB
    0x194C,  //10.0dB
    0x1ACC,  //10.5dB
    0x1C63,  //11.0dB
    0x1E11,  //11.5dB
    0x1FD9,  //12.0dB
    0x21BC,  //12.5dB
    0x23BC,  //13.0dB
    0x25DA,  //13.5dB
    0x2818,  //14.0dB
    0x2A79,  //14.5dB
    0x2CFD,  //15.0dB
    0x2FA7,  //15.5dB
    0x327A,  //16.0dB
};

const S32 DSPGAIN_Q11_32bit[140] =
{
    0x00000000,     //for mute
    0x00020C49,     //-60.0dB
    0x0002E493,     //-57.0dB
    0x00041617,     //-54.0dB
    0x000524F3,     //-52.0dB
    0x0007443E,     //-49.0dB
    0x0008273A,     //-48.0dB
    0x000925E8,     //-47.0dB
    0x000A43AA,     //-46.0dB
    0x000B8449,     //-45.0dB
    0x000CEC08,     //-44.0dB
    0x000E7FAC,     //-43.0dB
    0x00104491,     //-42.0dB
    0x001240B8,     //-41.0dB
    0x00147AE1,     //-40.0dB
    0x0016FA9B,     //-39.0dB
    0x0019C865,     //-38.0dB
    0x001CEDC3,     //-37.0dB
    0x001EA495,     //-36.5dB
    0x00207567,     //-36.0dB
    0x002261C4,     //-35.5dB
    0x00246B4E,     //-35.0dB
    0x002693BF,     //-34.5dB
    0x0028DCEB,     //-34.0dB
    0x002B48C4,     //-33.5dB
    0x002DD958,     //-33.0dB
    0x003090D3,     //-32.5dB
    0x00337184,     //-32.0dB
    0x00367DDC,     //-31.5dB
    0x0039B871,     //-31.0dB
    0x003D2400,     //-30.5dB
    0x0040C371,     //-30.0dB
    0x004499D6,     //-29.5dB
    0x0048AA70,     //-29.0dB
    0x004CF8B4,     //-28.5dB
    0x00518847,     //-28.0dB
    0x00565D0A,     //-27.5dB
    0x005B7B15,     //-27.0dB
    0x0060E6C0,     //-26.5dB
    0x0066A4A5,     //-26.0dB
    0x006CB9A2,     //-25.5dB
    0x00732AE1,     //-25.0dB
    0x0079FDD9,     //-24.5dB
    0x00813856,     //-24.0dB
    0x0088E078,     //-23.5dB
    0x0090FCBF,     //-23.0dB
    0x0099940D,     //-22.5dB
    0x00A2ADAD,     //-22.0dB
    0x00AC5156,     //-21.5dB
    0x00B68737,     //-21.0dB
    0x00C157FA,     //-20.5dB
    0x00CCCCCC,     //-20.0dB
    0x00D8EF66,     //-19.5dB
    0x00E5CA14,     //-19.0dB
    0x00F367BE,     //-18.5dB
    0x0101D3F2,     //-18.0dB
    0x01111AED,     //-17.5dB
    0x012149A5,     //-17.0dB
    0x01326DD7,     //-16.5dB
    0x0144960C,     //-16.0dB
    0x0157D1AE,     //-15.5dB
    0x016C310E,     //-15.0dB
    0x0181C576,     //-14.5dB
    0x0198A135,     //-14.0dB
    0x01B0D7B1,     //-13.5dB
    0x01CA7D76,     //-13.0dB
    0x01E5A847,     //-12.5dB
    0x02026F30,     //-12.0dB
    0x0220EA9F,     //-11.5dB
    0x0241346F,     //-11.0dB
    0x02636807,     //-10.5dB
    0x0287A26C,     //-10.0dB
    0x02AE025C,     //-9.5dB
    0x02D6A866,     //-9.0dB
    0x0301B70A,     //-8.5dB
    0x032F52CF,     //-8.0dB
    0x035FA26A,     //-7.5dB
    0x0392CED8,     //-7.0dB
    0x03C90386,     //-6.5dB
    0x04026E73,     //-6.0dB
    0x043F4057,     //-5.5dB
    0x047FACCF,     //-5.0dB
    0x04C3EA83,     //-4.5dB
    0x050C335D,     //-4.0dB
    0x0558C4B2,     //-3.5dB
    0x05A9DF7A,     //-3.0dB
    0x05FFC889,     //-2.5dB
    0x065AC8C2,     //-2.0dB
    0x06BB2D60,     //-1.5dB
    0x0721482B,     //-1.0dB
    0x078D6FC9,     //-0.5dB
    0x08000000,     //0.0dB
    0x08795A04,     //0.5dB
    0x08F9E4CF,     //1.0dB
    0x09820D74,     //1.5dB
    0x0A12477C,     //2.0dB
    0x0AAB0D48,     //2.5dB
    0x0B4CE07B,     //3.0dB
    0x0BF84A66,     //3.5dB
    0x0CADDC7B,     //4.0dB
    0x0D6E30CD,     //4.5dB
    0x0E39EA8E,     //5.0dB
    0x0F11B69D,     //5.5dB
    0x0FF64C16,     //6.0dB
    0x10E86CF1,     //6.5dB
    0x11E8E6A0,     //7.0dB
    0x12F892C7,     //7.5dB
    0x141857E9,     //8.0dB
    0x15492A38,     //8.5dB
    0x168C0C59,     //9.0dB
    0x17E21048,     //9.5dB
    0x194C583A,     //10.0dB
    0x1ACC179A,     //10.5dB
    0x1C629405,     //11.0dB
    0x1E112669,     //11.5dB
    0x1FD93C1F,     //12.0dB
    0x21BC5829,     //12.5dB
    0x23BC1478,     //13.0dB
    0x25DA2345,     //13.5dB
    0x28185086,     //14.0dB
    0x2A78836F,     //14.5dB
    0x2CFCC016,     //15.0dB
    0x2FA72923,     //15.5dB
    0x327A01A4,     //16.0dB
    0x3577AEF5,     //16.5dB
    0x38A2BACB,     //17.0dB
    0x3BFDD55A,     //17.5dB
    0x3F8BD79D,     //18.0dB
    0x434FC5C2,     //18.5dB
    0x474CD1B7,     //19.0dB
    0x4B865DE3,     //19.5dB
    0x50000000,     //20.0dB
    0x54BD842B,     //20.5dB
    0x59C2F01D,     //21.0dB
    0x5F14868E,     //21.5dB
    0x64B6CADC,     //22.0dB
    0x6AAE84D8,     //22.5dB
    0x7100C4D7,     //23.0dB
    0x77B2E7FF,     //23.5dB
    0x7ECA9CD2,     //24.0dB
};

typedef enum DSP_ANA_GAIN_IDX_e
{
	ANA_OUT_GAIN_NEG_36_DB,
	ANA_OUT_GAIN_NEG_33_DB,
	ANA_OUT_GAIN_NEG_30_DB,
	ANA_OUT_GAIN_NEG_27_DB,
	ANA_OUT_GAIN_NEG_24_DB,
	ANA_OUT_GAIN_NEG_21_DB,
	ANA_OUT_GAIN_NEG_18_DB,
	ANA_OUT_GAIN_NEG_15_DB,
	ANA_OUT_GAIN_NEG_12_DB,
	ANA_OUT_GAIN_NEG_9_DB,
	ANA_OUT_GAIN_NEG_6_DB,
	ANA_OUT_GAIN_NEG_5_DB,
	ANA_OUT_GAIN_NEG_4_DB,
	ANA_OUT_GAIN_NEG_3_DB,
	ANA_OUT_GAIN_NEG_2_DB,
	ANA_OUT_GAIN_NEG_1_DB,
	ANA_OUT_GAIN_0_DB,
	ANA_OUT_GAIN_POS_1_DB,
	ANA_OUT_GAIN_POS_2_DB,
	ANA_OUT_GAIN_POS_3_DB,
	ANA_OUT_GAIN_POS_6_DB,
	ANA_OUT_GAIN_POS_9_DB,
	ANA_OUT_GAIN_MAX_NO,
} DSP_ANA_GAIN_IDX_t;


const S32 DSP_ANA_GAIN_TABLE[] =
{
	0x00000000, // -36 dB
	0x00000001, // -33 dB
	0x00000003, // -30 dB
	0x00000007, // -27 dB
	0x0000000F, // -24 dB
	0x0000001F, // -21 dB
	0x0000003F, // -18 dB
	0x0000007F, // -15 dB
	0x000000FF, // -12 dB
	0x000001FF, // - 9 dB
	0x000003FF, // - 6 dB
	0x000007FF, // - 5 dB
	0x00000FFF, // - 4 dB
	0x00001FFF, // - 3 dB
	0x00003FFF, // - 2 dB
	0x00007FFF, // - 1 dB
	0x0000FFFF, //   0 dB
	0x0001FFFF, //   1 dB
	0x0003FFFF, //   2 dB
	0x0007FFFF, //   3 dB
	0x000FFFFF, //   6 dB
	0x001FFFFF, //   9 dB
};

typedef enum DSP_AU_CTRL_CH_e
{
	AUDIO_CTRL_CHANNEL_L,
	AUDIO_CTRL_CHANNEL_R,
} DSP_AU_CTRL_CH_t;

#define TOTAL_ANALOG_OUT_GAIN_STEP (sizeof(DSP_ANA_GAIN_TABLE)/sizeof(DSP_ANA_GAIN_TABLE[0]))


/******************************************************************************
 * Function Prototypes
 ******************************************************************************/
VOID DSP_GC_Init (VOID);
VOID DSP_GC_LoadAnalogOutGainTableByAudioComponent (AUDIO_GAIN_COMPONENT_t Component);
VOID DSP_GC_LoadAnalogInGainTableByAudioComponent (AUDIO_GAIN_COMPONENT_t Component);
VOID DSP_GC_LoadAInPGTableByAudioComponent (AUDIO_GAIN_COMPONENT_t Component);
VOID DSP_GC_LoadDigitalInGainTableByAudioComponent (AUDIO_GAIN_COMPONENT_t Component);
VOID DSP_GC_LoadDInPGTableByAudioComponent (AUDIO_GAIN_COMPONENT_t Component);
S16 DSP_GC_ConvertQ11Form (S16 Value);
S32 DSP_GC_ConvertQ11Form_32bit (S16 Value);
S16 DSP_GC_ConvertPercentageToIdx (S16 Percentage, S16 TotalIndex);

/* Digital Out */
S16 DSP_GC_GetDigitalGainIndexFromComponent(AUDIO_GAIN_COMPONENT_t Component, S16 TotalIndex);
S16 DSP_GC_GetDigitalOutLevel (AUDIO_GAIN_COMPONENT_t Component);
VOID DSP_GC_SetDigitalOutLevel (AUDIO_GAIN_COMPONENT_t Component, S16 Gain);
S16 DSP_GC_GetDigitalOutGain_AT (VOID);

/* Digital In */
S16 DSP_GC_GetDigitalInLevel (VOID);
VOID DSP_GC_SetDigitalInLevel (S16 PercentageGain);
S16 DSP_GC_GetDigitalInLevelByPercentageTable (VOID);
VOID DSP_GC_SetDigitalInLevelByGainTable (AUDIO_GAIN_COMPONENT_t Component);
VOID DSP_GC_LoadDigitalInLevelToDfe (VOID);

/* Analog Out */
S16 DSP_GC_GetAnalogOutLevel (VOID);
VOID DSP_GC_SetAnalogOutLevel (S16 PercentageGain);
VOID DSP_GC_SetAnalogOutScaleByGainTable (AUDIO_GAIN_COMPONENT_t Component);
VOID DSP_GC_LoadAnalogOutLevelToAfe (VOID);

VOID DSP_GC_SetAnalogOutOnSequence (VOID);
VOID DSP_GC_SetAnalogOutOffSequence (VOID);
DSP_ANA_GAIN_IDX_t DSP_GC_GetCurrTabIdx (VOID);
DSP_ANA_GAIN_IDX_t DSP_GC_SearchAnalogGainTableIndex (S32 Gain);
VOID DSP_GC_SetToTargetGainByStepFunction (DSP_ANA_GAIN_IDX_t InitialTabIdx, DSP_ANA_GAIN_IDX_t TargetTabIdx);

/* Analog In */
S16 DSP_GC_GetAnalogInLevel (AU_AFE_IN_GAIN_COMPONENT_t AfeComponent);
VOID DSP_GC_SetAnalogInLevel (AU_AFE_IN_GAIN_COMPONENT_t AfeComponent, S16 Gain);
VOID DSP_GC_SetAnalogInLevelByGainTable (AUDIO_GAIN_COMPONENT_t Component);
S16 DSP_GC_GetAnalogInLevelByPercentageTable (DSP_AU_CTRL_CH_t Channel);
VOID DSP_GC_LoadAnalogInLevelToAfe (AU_AFE_IN_GAIN_COMPONENT_t AfeComponent);
VOID DSP_GC_LoadAnalogInLevelToAfeConcurrently (VOID);

/* Overall */
VOID DSP_GC_SetDefaultLevelByGainTable (AUDIO_GAIN_COMPONENT_t Component);
VOID DSP_GC_LoadDefaultGainToAfe (VOID);
VOID DSP_GC_SetAfeGainLevelByPercentage (S16 PercentageGain);
VOID DSP_GC_UpdateAfeGains(S16 PercentageGain);


/**
 * DSP_GC_Init
 *
 * Initialization of Gain paramters
 *
 */
VOID DSP_GC_Init (VOID)
{
	U16 idx;

	configASSERT(ANA_OUT_GAIN_MAX_NO == TOTAL_ANALOG_OUT_GAIN_STEP);

	for (idx = 0 ; idx < AUDIO_GAIN_MAX_COMPONENT ; idx++)
	{
		gAudioCtrl.Gc.DigitalOut.Reg[idx] 								= 100;
	}

	gAudioCtrl.Gc.DigitalOut.Reg[AUDIO_GAIN_VP] 						= 70;
	gAudioCtrl.Gc.DigitalOut.Reg[AUDIO_GAIN_RT] 						= 70;

	gAudioCtrl.Gc.AnalogOut												= 100;

	gAudioCtrl.Gc.GainTable.DigitalIn.Field.Gain 						= 0;

	gAudioCtrl.Gc.GainTable.AnalogIn.Field.LineGain_L					= 0;			/* 0x5E */
	gAudioCtrl.Gc.GainTable.AnalogIn.Field.LineGain_R					= 0;			/* 0x5E */
	gAudioCtrl.Gc.GainTable.AnalogIn.Field.MicGain_L					= 0;			/* 0x5E */
	gAudioCtrl.Gc.GainTable.AnalogIn.Field.MicGain_R					= 0;			/* 0x5E */
	gAudioCtrl.Gc.GainTable.AnalogIn.Field.AncMicGain_L					= 0x01;			/* 0x65 */
	gAudioCtrl.Gc.GainTable.AnalogIn.Field.AncMicGain_R					= 0x01;			/* 0x65 */
	gAudioCtrl.Gc.GainTable.AnalogIn.Field.VadMicGain					= 0x10;			/* 0x67 */

	// gAudioCtrl.Gc.StaticControl.TablePtr								= pvPortMalloc(sizeof(DSP_STATIC_PERCENTAGE_GAIN_TABLE_t));

	// gAudioCtrl.Gc.StaticControl.TablePtr->InputPercentCtrl.Enable		= FALSE;
	// gAudioCtrl.Gc.StaticControl.TablePtr->InputPercentCtrl.Percentage	= 50;

	// DSP_GC_SetDefaultLevelByGainTable(AUDIO_GAIN_A2DP);
	// DSP_GC_LoadDefaultGainToAfe();

    hal_audio_set_gain_parameters(0, 0, 4, 4);

}



/**
 * DSP_GC_LoadAnalogOutGainTableByAudioComponent
 *
 * Load Gain Table by Audio Component
 *
 */
VOID DSP_GC_LoadAnalogOutGainTableByAudioComponent (AUDIO_GAIN_COMPONENT_t Component)
{
/*
	S16 KeyId;

	switch (Component)
	{
		case AUDIO_GAIN_A2DP:
			KeyId = NVKEY_DSP_PARA_ANALOG_GAINTABLE_A2DP;
			break;

		case AUDIO_GAIN_LINE:
			KeyId = NVKEY_DSP_PARA_ANALOG_GAINTABLE_LINE;
			break;

		case AUDIO_GAIN_SCO:
			KeyId = NVKEY_DSP_PARA_ANALOG_GAINTABLE_SCO;
			break;

		case AUDIO_GAIN_SCO_NB:
			KeyId = NVKEY_DSP_PARA_ANALOG_GAINTABLE_SCO_NB;
			break;

		case AUDIO_GAIN_VC:
			KeyId = NVKEY_DSP_PARA_ANALOG_GAINTABLE_VC;
			break;

		case AUDIO_GAIN_VP:
			KeyId = NVKEY_DSP_PARA_ANALOG_GAINTABLE_VP;
			break;

		case AUDIO_GAIN_RT:
			KeyId = NVKEY_DSP_PARA_ANALOG_GAINTABLE_RT;
			break;

		case AUDIO_GAIN_AT:
			KeyId = NVKEY_DSP_PARA_ANALOG_GAINTABLE_AT;
			break;

		default:
			return;
	}

	NVKEY_ReadFullKey(KeyId,
				      &gAudioCtrl.Gc.StaticControl.TablePtr->AOutPercentage,
				      sizeof(DSP_ANALOG_OUT_GAIN_TABLE_CTRL_t));
				      */
    UNUSED(Component);
}



/**
 * DSP_GC_LoadAnalogInGainTableByAudioComponent
 *
 * Load Gain Table by Audio Component
 *
 */
VOID DSP_GC_LoadAnalogInGainTableByAudioComponent (AUDIO_GAIN_COMPONENT_t Component)
{
/*
	S16 KeyId;

	switch (Component)
	{
		case AUDIO_GAIN_A2DP:
			KeyId = NVKEY_DSP_PARA_AIN_GAINTABLE_A2DP;
			break;

		case AUDIO_GAIN_LINE:
			KeyId = NVKEY_DSP_PARA_AIN_GAINTABLE_LINE;
			break;

		case AUDIO_GAIN_SCO:
			KeyId = NVKEY_DSP_PARA_AIN_GAINTABLE_SCO;
			break;

		case AUDIO_GAIN_SCO_NB:
			KeyId = NVKEY_DSP_PARA_AIN_GAINTABLE_SCO_NB;
			break;

		case AUDIO_GAIN_VC:
			KeyId = NVKEY_DSP_PARA_AIN_GAINTABLE_VC;
			break;

		case AUDIO_GAIN_VP:
			KeyId = NVKEY_DSP_PARA_AIN_GAINTABLE_VP;
			break;

		case AUDIO_GAIN_RT:
			KeyId = NVKEY_DSP_PARA_AIN_GAINTABLE_RT;
			break;

		case AUDIO_GAIN_AT:
			KeyId = NVKEY_DSP_PARA_AIN_GAINTABLE_AT;
			break;

		default:
			return;
	}

	NVKEY_ReadFullKey(KeyId,
				      &gAudioCtrl.Gc.GainTable.AnalogIn,
				      sizeof(DSP_GAIN_ANA_IN_CTRL_t));
				      */
    UNUSED(Component);
}


/**
 * DSP_GC_LoadAInPGTableByAudioComponent
 *
 * Load Analog Input Percentage Gain Table by Audio Component
 *
 */
VOID DSP_GC_LoadAInPGTableByAudioComponent (AUDIO_GAIN_COMPONENT_t Component)
{
/*
	S16 KeyIdL, KeyIdR;

	switch (Component)
	{
		case AUDIO_GAIN_SCO:
			KeyIdL = NVKEY_DSP_PARA_AIN_GP_TABLE_SCO_L;
			KeyIdR = NVKEY_DSP_PARA_AIN_GP_TABLE_SCO_R;
			break;

		case AUDIO_GAIN_SCO_NB:
			KeyIdL = NVKEY_DSP_PARA_AIN_GP_TABLE_SCO_NB_L;
			KeyIdR = NVKEY_DSP_PARA_AIN_GP_TABLE_SCO_NB_R;
			break;

		case AUDIO_GAIN_AT:
			KeyIdL = NVKEY_DSP_PARA_AIN_GP_TABLE_AT_L;
			KeyIdR = NVKEY_DSP_PARA_AIN_GP_TABLE_AT_R;
			break;

		default:
			OS_ASSERT(FALSE); // should not enter
	}

	NVKEY_ReadFullKey(KeyIdL,
				      &gAudioCtrl.Gc.StaticControl.TablePtr->AInPercentageL,
				      sizeof(DSP_ANALOG_IN_GAIN_TABLE_CTRL_t));

	NVKEY_ReadFullKey(KeyIdR,
				      &gAudioCtrl.Gc.StaticControl.TablePtr->AInPercentageR,
				      sizeof(DSP_ANALOG_IN_GAIN_TABLE_CTRL_t));
				      */
    UNUSED(Component);
}



/**
 * DSP_GC_LoadDigitalInGainTableByAudioComponent
 *
 * Load Gain Table by Audio Component
 *
 */
VOID DSP_GC_LoadDigitalInGainTableByAudioComponent (AUDIO_GAIN_COMPONENT_t Component)
{
/*
	S16 KeyId;

	switch (Component)
	{
		case AUDIO_GAIN_A2DP:
			KeyId = NVKEY_DSP_PARA_DIN_GAINTABLE_A2DP;
			break;

		case AUDIO_GAIN_LINE:
			KeyId = NVKEY_DSP_PARA_DIN_GAINTABLE_LINE;
			break;

		case AUDIO_GAIN_SCO:
		case AUDIO_GAIN_SCO_NB:
			KeyId = NVKEY_DSP_PARA_DIN_GAINTABLE_SCO;
			break;

		case AUDIO_GAIN_VC:
			KeyId = NVKEY_DSP_PARA_DIN_GAINTABLE_VC;
			break;

		case AUDIO_GAIN_VP:
			KeyId = NVKEY_DSP_PARA_DIN_GAINTABLE_VP;
			break;

		case AUDIO_GAIN_RT:
			KeyId = NVKEY_DSP_PARA_DIN_GAINTABLE_RT;
			break;

		case AUDIO_GAIN_AT: //Falls through
			KeyId = NVKEY_DSP_PARA_DIN_GAINTABLE_AT;
			break;

		default:
			return;
	}

	NVKEY_ReadFullKey(KeyId,
				      &gAudioCtrl.Gc.GainTable.DigitalIn.Field.Gain,
				      sizeof(S16));
				      */
    UNUSED(Component);
}


/**
 * DSP_GC_LoadDInPGTableByAudioComponent
 *
 * Load Analog Input Percentage Gain Table by Audio Component
 *
 */
VOID DSP_GC_LoadDInPGTableByAudioComponent (AUDIO_GAIN_COMPONENT_t Component)
{
/*
	S16 KeyId;

	switch (Component)
	{
		case AUDIO_GAIN_SCO:
			KeyId = NVKEY_DSP_PARA_DIN_GP_TABLE_SCO;
			break;

		case AUDIO_GAIN_SCO_NB:
			KeyId = NVKEY_DSP_PARA_DIN_GP_TABLE_SCO_NB;
			break;

		case AUDIO_GAIN_AT:
			KeyId = NVKEY_DSP_PARA_DIN_GP_TABLE_AT;
			break;

		default:
			OS_ASSERT(FALSE); // should not enter
	}

	NVKEY_ReadFullKey(KeyId,
				      &gAudioCtrl.Gc.StaticControl.TablePtr->DInPercentage,
				      sizeof(DSP_DIGITAL_IN_GAIN_TABLE_CTRL_t));
				      */
    UNUSED(Component);
}


/**
 * DSP_GC_ConvertQ11Form
 *
 * Convert table value to Q11 format
 *
 */
S16 DSP_GC_ConvertQ11Form (S16 Value)
{
    configASSERT(Value < (S16)Q11_TABLE_SIZE);
    return DSPGAIN_Q11[Value];
}
S32 DSP_GC_ConvertQ11Form_32bit (S16 Value)
{
    configASSERT(Value < (S16)Q11_TABLE_SIZE);
    return DSPGAIN_Q11_32bit[Value];
}


/**
 * DSP_GC_ConvertPercentageToIdx
 *
 * Convert Gain Percentage To Gain Index
 *
 */
S16 DSP_GC_ConvertPercentageToIdx (S16 Percentage, S16 TotalIndex)
{
	S16 Index;

	Index = (TotalIndex * Percentage) / 100;

	if (Index == TotalIndex)
	{
		Index = (TotalIndex - 1);
	}

	return Index;
}


/**
 * DSP_GC_GetDigitalGainIndexFromComponent
 *
 * Get Gain Index From Gain Component
 *
 */
S16 DSP_GC_GetDigitalGainIndexFromComponent(AUDIO_GAIN_COMPONENT_t Component, S16 TotalIndex)
{
	return (DSP_GC_ConvertPercentageToIdx(DSP_GC_GetDigitalOutLevel(Component),TotalIndex));
}


/**
 * DSP_GC_GetDigitalOutLevel
 *
 * Get digital output gain
 *
 */
S16 DSP_GC_GetDigitalOutLevel (AUDIO_GAIN_COMPONENT_t Component)
{
	return gAudioCtrl.Gc.DigitalOut.Reg[Component];
}


/**
 * DSP_GC_SetDigitalOutLevel
 *
 * Set digital output gain
 *
 */
VOID DSP_GC_SetDigitalOutLevel (AUDIO_GAIN_COMPONENT_t Component, S16 Gain)
{
	configASSERT(Gain <= 100);

	switch (Component)
	{
		case AUDIO_GAIN_A2DP: 	// Falls through
		case AUDIO_GAIN_VP:  	// Falls through
		case AUDIO_GAIN_LINE:   // Falls through
		case AUDIO_GAIN_SCO:   	// Falls through
		case AUDIO_GAIN_VC:   	// Falls through
		case AUDIO_GAIN_RT:		// Falls through
		case AUDIO_GAIN_AT:
			gAudioCtrl.Gc.DigitalOut.Reg[Component] = Gain;
			/*
			logPrint(LOG_DSP,
					 PRINT_LEVEL_INFO,
					 DSP_INFO_DigitalOutputGainString,
					 2,
					 (U32)Component,
					 (U32)Gain);
			if (AUDIO_GAIN_AT == Component)
			{
				MDSP_AT_UpdateGain();
			}
			*/
		default:
			break;
	}
}


/**
 * DSP_GC_GetDigitalOutGain_AT
 *
 * API to get digital out value in Q11
 *
 */
S16 DSP_GC_GetDigitalOutGain_AT (VOID)
{
	/*
	DSP_OUT_GAIN_TABLE_CTRL_t* pAddr;

	pAddr = (DSP_OUT_GAIN_TABLE_CTRL_t*)NVKEY_GetPayloadFlashAddress(NVKEY_DSP_PARA_DIGITAL_GAINTABLE_AT);

	S16 TotalGainIndex = pAddr->TotalIndex;
	S16 GainIdx = DSP_GC_GetDigitalGainIndexFromComponent(AUDIO_GAIN_AT, TotalGainIndex);

	return DSP_GC_ConvertQ11Form(pAddr->OutGainIndex[GainIdx]);
	*/
	return 0;
}


/**
 * DSP_GC_GetDigitalInLevel
 *
 * Set digital input gain
 *
 */
S16 DSP_GC_GetDigitalInLevel (VOID)
{
	return gAudioCtrl.Gc.GainTable.DigitalIn.Field.Gain;
}


/**
 * DSP_GC_SetDigitalInLevel
 *
 * Set digital input gain
 *
 */
VOID DSP_GC_SetDigitalInLevel (S16 PercentageGain)
{
	if ((PercentageGain <= 100) && (PercentageGain >= 0))
	{
		gAudioCtrl.Gc.GainTable.DigitalIn.Field.Gain = PercentageGain;
	}
	else
	{
		// warning
	}
}


/**
 * DSP_GC_GetDigitalInLevelByPercentageTable
 *
 * Get digital input gain from Percentage Table
 *
 */
S16 DSP_GC_GetDigitalInLevelByPercentageTable (VOID)
{
	S32 Gain, GainIdx;
	S16 GainPercentage;

	GainPercentage = gAudioCtrl.Gc.StaticControl.TablePtr->InputPercentCtrl.Percentage;

	GainIdx
		= DSP_GC_ConvertPercentageToIdx(GainPercentage,
										gAudioCtrl.Gc.StaticControl.TablePtr->DInPercentage.TotalIndex);

	Gain = gAudioCtrl.Gc.StaticControl.TablePtr->DInPercentage.GainIndex[GainIdx];

	return Gain;
}


/**
 * DSP_GC_SetDigitalInLevelByGainTable
 *
 * Set digital input gain table
 *
 */
VOID DSP_GC_SetDigitalInLevelByGainTable (AUDIO_GAIN_COMPONENT_t Component)
{
	if (gAudioCtrl.Gc.StaticControl.TablePtr->InputPercentCtrl.Enable)
 	{
 		DSP_GC_LoadDInPGTableByAudioComponent(Component);
		gAudioCtrl.Gc.GainTable.DigitalIn.Field.Gain
			= DSP_GC_GetDigitalInLevelByPercentageTable();
 	}
 	else
 	{
		DSP_GC_LoadDigitalInGainTableByAudioComponent(Component);
 	}
}


/**
 * DSP_GC_LoadDigitalInLevelToDfe
 *
 * Set digital input gain to dfe
 *
 */
VOID DSP_GC_LoadDigitalInLevelToDfe (VOID)
{
	S16 Gain;

	Gain = DSP_GC_GetDigitalInLevel();
/*
	logPrint(LOG_DSP,
             PRINT_LEVEL_INFO,
             DSP_INFO_DigitalInputGainString,
             1,
             (U32)Gain);

	if ((Gain < 8) && (Gain >= 0))
	{
		AUDIO_CODEC.DWN_FIL_SET0.field.DEC1_FIL_DIG_GAIN = Gain;
	}
	else
	{
		// warning
	}
	*/
}


/**
 * DSP_GC_GetAnalogOutLevel
 *
 * Set analog output gain
 *
 */
S16 DSP_GC_GetAnalogOutLevel (VOID)
{
	return gAudioCtrl.Gc.AnalogOut;
}


/**
 * DSP_GC_SetAnalogIOutLevel
 *
 * Set analog output gain
 *
 */
VOID DSP_GC_SetAnalogOutLevel (S16 PercentageGain)
{
	if ((PercentageGain <= 100) && (PercentageGain >= 0))
	{
		gAudioCtrl.Gc.AnalogOut = PercentageGain;
	}
	else
	{
		// warning
	}
}


/**
 * DSP_GC_SetAnalogOutScaleByGainTable
 *
 * Set analog output gain table
 *
 */
VOID DSP_GC_SetAnalogOutScaleByGainTable (AUDIO_GAIN_COMPONENT_t Component)
{
	if (Component < AUDIO_GAIN_MAX_COMPONENT)
	{
		DSP_GC_LoadAnalogOutGainTableByAudioComponent(Component);
	}
}


/**
 * DSP_GC_LoadAnalogOutLevelToAfe
 *
 * Set analog output gain to afe
 *
 */
VOID DSP_GC_LoadAnalogOutLevelToAfe (VOID)
{
	S32 Gain, GainIdx;
	S16 GainPercentage;

	GainPercentage = DSP_GC_GetAnalogOutLevel();

	GainIdx
		= DSP_GC_ConvertPercentageToIdx(GainPercentage,
										gAudioCtrl.Gc.StaticControl.TablePtr->AOutPercentage.TotalIndex);

	Gain = gAudioCtrl.Gc.StaticControl.TablePtr->AOutPercentage.GainIndex[GainIdx];
/*
	logPrint(LOG_DSP,
			 PRINT_LEVEL_INFO,
			 DSP_INFO_AnalogOutputGainString,
			 1,
			 (U32)Gain);
*/
	DSP_DRV_AFE_SetOutputGain(AU_AFE_OUT_COMPONENT_EAR_GAIN, Gain);
}


/**
 * DSP_GC_ResetAnalogOutGain
 *
 * Reset analog output to -6 dB
 *
 */
VOID DSP_GC_ResetAnalogOutGain (VOID)
{
	DSP_DRV_AFE_SetOutputGain(AU_AFE_OUT_COMPONENT_EAR_GAIN, DSP_ANA_GAIN_TABLE[ANA_OUT_GAIN_NEG_6_DB]);
}


/**
 * DSP_GC_SetAnalogOutOnSequence
 *
 * Set analog output audio on sequence
 *
 */
VOID DSP_GC_SetAnalogOutOnSequence (VOID)
{
	DSP_ANA_GAIN_IDX_t TargetTabIdx = DSP_GC_GetCurrTabIdx();

	DSP_GC_SetToTargetGainByStepFunction(ANA_OUT_GAIN_NEG_6_DB, TargetTabIdx);
}


/**
 * DSP_GC_SetAnalogOutOffSequence
 *
 * Set analog output audio off sequence
 *
 */
VOID DSP_GC_SetAnalogOutOffSequence (VOID)
{
	DSP_ANA_GAIN_IDX_t CurrTabIdx = DSP_GC_GetCurrTabIdx();

	DSP_GC_SetToTargetGainByStepFunction(CurrTabIdx, ANA_OUT_GAIN_NEG_6_DB);
}


/**
 * DSP_GC_GetCurrTabIdx
 *
 * Get current table index by current gain level
 *
 */
DSP_ANA_GAIN_IDX_t DSP_GC_GetCurrTabIdx (VOID)
{
	S32 Gain, GainIdx;
	S16 GainPercentage;
	DSP_ANA_GAIN_IDX_t CurrTabIdx;

	GainPercentage = DSP_GC_GetAnalogOutLevel();

	GainIdx
		= DSP_GC_ConvertPercentageToIdx(GainPercentage,
										gAudioCtrl.Gc.StaticControl.TablePtr->AOutPercentage.TotalIndex);

	Gain = gAudioCtrl.Gc.StaticControl.TablePtr->AOutPercentage.GainIndex[GainIdx];

	CurrTabIdx = DSP_GC_SearchAnalogGainTableIndex(Gain);

	return CurrTabIdx;
}


/**
 * DSP_GC_SearchAnalogGainTableIndex
 *
 * Search for corresponding gain index in table from input Gain
 *
 */
DSP_ANA_GAIN_IDX_t DSP_GC_SearchAnalogGainTableIndex (S32 Gain)
{
	DSP_ANA_GAIN_IDX_t Idx = 0;

	while (Idx < TOTAL_ANALOG_OUT_GAIN_STEP)
	{
		if (Gain > DSP_ANA_GAIN_TABLE[Idx])
		{
			Idx++;
		}
		else
		{
			break;
		}
	}

	return Idx;
}


/**
 * DSP_GC_SetToTargetGainByStepFunction
 *
 * Set analog output gain from InitialTabIdx to TargetTabIdx
 *
 */
VOID DSP_GC_SetToTargetGainByStepFunction (DSP_ANA_GAIN_IDX_t InitialTabIdx, DSP_ANA_GAIN_IDX_t TargetTabIdx)
{
	DSP_ANA_GAIN_IDX_t TabIdx = InitialTabIdx;

	if (TargetTabIdx > TabIdx)
	{
		for ( ; TabIdx <= TargetTabIdx ; TabIdx++)
		{
			DSP_DRV_AFE_SetOutputGain(AU_AFE_OUT_COMPONENT_EAR_GAIN, DSP_ANA_GAIN_TABLE[TabIdx]);
		}

	}
	else if (TargetTabIdx < TabIdx)
	{
		for ( ; TabIdx >= TargetTabIdx ; TabIdx--)
		{
			DSP_DRV_AFE_SetOutputGain(AU_AFE_OUT_COMPONENT_EAR_GAIN, DSP_ANA_GAIN_TABLE[TabIdx]);
		}
	}
	else
	{
		DSP_DRV_AFE_SetOutputGain(AU_AFE_OUT_COMPONENT_EAR_GAIN, DSP_ANA_GAIN_TABLE[TabIdx]);
	}

}


/**
 * DSP_GC_GetAnalogInLevel
 *
 * Get analog input gain
 *
 */
S16 DSP_GC_GetAnalogInLevel (AU_AFE_IN_GAIN_COMPONENT_t AfeComponent)
{
	if (AfeComponent < AU_AFE_IN_COMPONENT_NO)
	{
		return gAudioCtrl.Gc.GainTable.AnalogIn.Reg[AfeComponent];
	}

	return 0;
}


/**
 * DSP_GC_SetAnalogInLevel
 *
 * Set analog input gain
 *
 */
VOID DSP_GC_SetAnalogInLevel (AU_AFE_IN_GAIN_COMPONENT_t AfeComponent, S16 Gain)
{
	if (AfeComponent < AU_AFE_IN_COMPONENT_NO)
	{
		gAudioCtrl.Gc.GainTable.AnalogIn.Reg[AfeComponent] = Gain;
	}
}


/**
 * DSP_GC_GetAnalogInLevelByPercentageTable
 *
 * Get analog input gain from Percentage Table
 *
 */
S16 DSP_GC_GetAnalogInLevelByPercentageTable (DSP_AU_CTRL_CH_t Channel)
{
	S32 Gain, GainIdx;
	S16 GainPercentage;

	GainPercentage = gAudioCtrl.Gc.StaticControl.TablePtr->InputPercentCtrl.Percentage;

	if (Channel == AUDIO_CTRL_CHANNEL_L) //L
	{
		GainIdx
			= DSP_GC_ConvertPercentageToIdx(GainPercentage,
											gAudioCtrl.Gc.StaticControl.TablePtr->AInPercentageL.TotalIndex);

		Gain = gAudioCtrl.Gc.StaticControl.TablePtr->AInPercentageL.GainIndex[GainIdx];
	}
	else //R
	{
		GainIdx
			= DSP_GC_ConvertPercentageToIdx(GainPercentage,
											gAudioCtrl.Gc.StaticControl.TablePtr->AInPercentageR.TotalIndex);

		Gain = gAudioCtrl.Gc.StaticControl.TablePtr->AInPercentageR.GainIndex[GainIdx];
	}

	return Gain;
}


/**
 * DSP_GC_SetAnalogInLevelByGainTable
 *
 * Set analog input gain table
 *
 */
VOID DSP_GC_SetAnalogInLevelByGainTable (AUDIO_GAIN_COMPONENT_t Component)
{
	if (Component < AUDIO_GAIN_MAX_COMPONENT)
	{
		DSP_GC_LoadAnalogInGainTableByAudioComponent(Component);

		if (gAudioCtrl.Gc.StaticControl.TablePtr->InputPercentCtrl.Enable)
	 	{
	 		DSP_GC_LoadAInPGTableByAudioComponent(Component);

			gAudioCtrl.Gc.GainTable.AnalogIn.Field.MicGain_L
				= DSP_GC_GetAnalogInLevelByPercentageTable(AUDIO_CTRL_CHANNEL_L);

			gAudioCtrl.Gc.GainTable.AnalogIn.Field.MicGain_R
				= DSP_GC_GetAnalogInLevelByPercentageTable(AUDIO_CTRL_CHANNEL_R);
	 	}
	}
}


/**
 * DSP_GC_LoadAnalogInLevelToAfe
 *
 * Set analog input gain to afe
 *
 */
VOID DSP_GC_LoadAnalogInLevelToAfe (AU_AFE_IN_GAIN_COMPONENT_t AfeComponent)
{
	S16 Gain;

	if (AfeComponent < AU_AFE_IN_COMPONENT_NO)
	{
		Gain = DSP_GC_GetAnalogInLevel(AfeComponent);

		DSP_DRV_AFE_SetInputGain(AfeComponent, Gain);
	}
}


/**
 * DSP_GC_LoadAnalogInLevelToAfeConcurrently
 *
 * Set analog input gain from current settings concurrently
 *
 */
VOID DSP_GC_LoadAnalogInLevelToAfeConcurrently (VOID)
{
	U16 AfeComponent;

	for (AfeComponent = 0 ; AfeComponent < AU_AFE_IN_COMPONENT_NO ; AfeComponent++)
	{
		if ((AfeComponent == AU_AFE_IN_COMPONENT_ANC_MIC_L) || (AfeComponent == AU_AFE_IN_COMPONENT_ANC_MIC_R))
		{
			continue;
		}

		DSP_DRV_AFE_SetInputGain(AfeComponent, gAudioCtrl.Gc.GainTable.AnalogIn.Reg[AfeComponent]);
	}
}


/**
 * DSP_GC_SetDefaultLevelByGainTable
 *
 * Load Default by Audio Component
 *
 */
VOID DSP_GC_SetDefaultLevelByGainTable (AUDIO_GAIN_COMPONENT_t Component)
{
	if ((AUDIO_GAIN_SCO_NB == Component)
	 || (AUDIO_GAIN_SCO == Component))
	{
		#if 1 // Do not enable until verified
		gAudioCtrl.Gc.StaticControl.TablePtr->InputPercentCtrl.Enable = TRUE;
		#else
		gAudioCtrl.Gc.StaticControl.TablePtr->InputPercentCtrl.Enable = FALSE;
		#endif
	}
	else
	{
		gAudioCtrl.Gc.StaticControl.TablePtr->InputPercentCtrl.Enable = FALSE;
	}

	DSP_GC_SetAnalogOutScaleByGainTable(Component);
	DSP_GC_SetAnalogInLevelByGainTable(Component);
	DSP_GC_SetDigitalInLevelByGainTable(Component);
}


/**
 * DSP_GC_LoadDefaultGainToAfe
 *
 * Load Default Gain to Audio front end
 *
 */
VOID DSP_GC_LoadDefaultGainToAfe (VOID)
{
	DSP_GC_LoadDigitalInLevelToDfe();
	DSP_GC_LoadAnalogOutLevelToAfe();
	DSP_GC_LoadAnalogInLevelToAfeConcurrently();
}


/**
 * DSP_GC_SetAfeGainLevelByPercentage
 *
 * Update audio front end gain level by percentage
 *
 */
VOID DSP_GC_SetAfeGainLevelByPercentage (S16 PercentageGain)
{
	if ((PercentageGain <= 100) && (PercentageGain >= 0)
	 && (gAudioCtrl.Gc.StaticControl.TablePtr->InputPercentCtrl.Enable))
	{
		gAudioCtrl.Gc.StaticControl.TablePtr->InputPercentCtrl.Percentage = PercentageGain;

		gAudioCtrl.Gc.GainTable.DigitalIn.Field.Gain
			= DSP_GC_GetDigitalInLevelByPercentageTable();

		gAudioCtrl.Gc.GainTable.AnalogIn.Field.MicGain_L
			= DSP_GC_GetAnalogInLevelByPercentageTable(AUDIO_CTRL_CHANNEL_L);

		gAudioCtrl.Gc.GainTable.AnalogIn.Field.MicGain_R
			= DSP_GC_GetAnalogInLevelByPercentageTable(AUDIO_CTRL_CHANNEL_R);
	}
	else
	{
		// warning
	}
}


/**
 * DSP_GC_UpdateAfeGains
 *
 * Update all gain levels by percentage
 *
 */
VOID DSP_GC_UpdateAfeGains(S16 PercentageGain)
{
	/* Update Analog In & Digital In Level */
	DSP_GC_SetAfeGainLevelByPercentage(PercentageGain);

	/* Update Analog Out Level */
	DSP_GC_SetAnalogOutLevel(PercentageGain);

	/* Update Set Afe Level to Hardware */
	DSP_GC_LoadDefaultGainToAfe();
}


/**
 * DSP_GC_MuteAudioSource
 *
 * API to mute audio source
 *
 * @Ctrl: TRUE for mute, FALSE for unmute
 *
 */
VOID DSP_GC_MuteAudioSource (BOOL Ctrl)
{
	Source_Audio_Configuration(0, AUDIO_SOURCE_MUTE_ENABLE, Ctrl);
}


/**
 * DSP_GC_MuteAudioSink
 *
 * API to mute audio sink
 *
 * @Ctrl: TRUE for mute, FALSE for unmute
 *
 */
VOID DSP_GC_MuteAudioSink (BOOL Ctrl)
{
	Sink_Audio_Configuration(Sink_blks[SINK_TYPE_AUDIO], AUDIO_SINK_MUTE_ENABLE, Ctrl);
}


/*Set volume from CM4 ccni msg*/

void DSP_GC_SetOutputVolume(hal_ccni_message_t msg, hal_ccni_message_t *ack)
{
    uint16_t output_index = (uint16_t)(msg.ccni_message[0]&0xFFFF);
    uint16_t d_gain_index = (uint16_t)msg.ccni_message[1];
    uint16_t a_gain_index = (uint32_t)(msg.ccni_message[1] >> 16);

    if (output_index == HAL_AUDIO_OUTPUT_GAIN_SELECTION_D0_ALR) {
        //DL1
        hal_audio_set_stream_out_volume(d_gain_index, a_gain_index);
    } else if (output_index == HAL_AUDIO_OUTPUT_GAIN_SELECTION_D1_ALR) {
        //DL2
        hal_audio_set_stream_out_volume2(d_gain_index, a_gain_index);
    } else if (output_index == HAL_AUDIO_OUTPUT_GAIN_SELECTION_D2_ALR) {
        //DL3
        hal_audio_set_stream_out_volume3(d_gain_index, a_gain_index);
#ifdef AIR_AUDIO_LR_OUT_ANALOG_GAIN_OFFSET_ENABLE
    } else if (output_index == HAL_AUDIO_OUTPUT_GAIN_SELECTION_AOL_AOR) {
        //DL offset a gain
        hal_audio_set_stream_out_offset_volume(d_gain_index, a_gain_index);
#endif
    } else {
        DSP_MW_LOG_E("DSP_GC_SetOutputVolume, %2d, index error", 1, output_index);
    }
    DSP_MW_LOG_I("DSP_GC_SetOutputVolume,%2d: a=0x%x, d=0x%x", 3, output_index ,a_gain_index, d_gain_index);
    UNUSED(ack);
}

void DSP_GC_SetInputVolume(hal_ccni_message_t msg, hal_ccni_message_t *ack)
{
    hal_audio_stream_in_scenario_t type;
    uint16_t input_index = (uint16_t)(msg.ccni_message[0]&0xFFFF);
    int16_t gain_index0 = (int16_t)msg.ccni_message[1];
    uint16_t gain_index1 = (uint32_t)(msg.ccni_message[1] >> 16);

    type = (input_index >> 8) & 0x7F;
    if ((input_index & HAL_AUDIO_STREAM_IN_SCENARIO_MARK) && (type < HAL_AUDIO_STREAM_IN_SCENARIO_MAX)) {
#ifdef MTK_BT_HFP_ENABLE
#ifdef AIR_SOFTWARE_GAIN_ENABLE
        if ((type == HAL_AUDIO_STREAM_IN_SCENARIO_HFP)||(type == HAL_AUDIO_STREAM_IN_SCENARIO_BLE_CALL)) {
            DSP_MW_LOG_I("CALL UL_SW_Gain_Set %d type %d", 2, gain_index0,type);
            Call_UL_SW_Gain_Set(gain_index0);
        }
#endif
#endif
    }

    input_index = input_index & 0xFF;
    if (input_index == HAL_AUDIO_INPUT_GAIN_SELECTION_D0_A0) {
        hal_audio_set_stream_in_volume(gain_index0, gain_index1);
        DSP_MW_LOG_I("DSP_GC_SetInputVolume,%2d: a=0x%x, d=0x%x", 3, input_index, gain_index1, gain_index0);
    } else {
        hal_audio_set_stream_in_volume_for_multiple_microphone(gain_index0, gain_index1, input_index);
        DSP_MW_LOG_I("DSP_GC_SetInputVolume,%2d: gain_index0=0x%x, gain_index1=0x%x", 3, input_index, gain_index0, gain_index1);
    }

    UNUSED(ack);
}

void DSP_GC_MuteOutput(hal_ccni_message_t msg, hal_ccni_message_t *ack)
{
    bool enable = (bool)(uint16_t)msg.ccni_message[1];
    hal_audio_hw_stream_out_index_t hw_gain_index = (hal_audio_hw_stream_out_index_t)(uint32_t)(msg.ccni_message[1] >> 16);
    DSP_MW_LOG_I("mute enable = %d, hw_gain_index = %d", 2, enable, hw_gain_index);
#if defined(HAL_AUDIO_SUPPORT_MULTIPLE_STREAM_OUT)
    hal_audio_mute_stream_out(enable, hw_gain_index);
#else
    hal_audio_mute_stream_out(enable);
#endif
    //AudioAfeConfiguration(AUDIO_SINK_MUTE_ENABLE, enable);
    UNUSED(ack);
}

void DSP_GC_SetGainParameters(hal_ccni_message_t msg, hal_ccni_message_t *ack)
{
    if ((msg.ccni_message[1] >> 16) != 0x5AA5 ) { // 0x5AA5 means DL4&DL3
        int16_t gain1_compensation = (int16_t)((msg.ccni_message[1])&0xFFFF);
        int16_t gain2_compensation = (int16_t)((msg.ccni_message[1]>>16)&0xFFFF);
        uint16_t gain1_per_step = (uint16_t)((msg.ccni_message[0])&0xFF);
        uint16_t gain2_per_step = (uint16_t)((msg.ccni_message[0]>>8)&0xFF);
        #if 0//modify for ab1568
        hal_audio_set_gain_parameters(gain1_compensation, gain2_compensation, gain1_per_step, gain2_per_step);
        #else
        hal_audio_volume_digital_gain_setting_parameter_t   digital_gain_setting;
        digital_gain_setting.index_compensation = (uint32_t)((int32_t)((int16_t)gain1_compensation));
        digital_gain_setting.memory_select = HAL_AUDIO_MEMORY_DL_DL1|HAL_AUDIO_MEMORY_DL_SRC1;
        digital_gain_setting.sample_per_step = (uint32_t)gain1_per_step;
        hal_audio_control_set_value((hal_audio_set_value_parameter_t *)&digital_gain_setting, HAL_AUDIO_SET_VOLUME_HW_DIGITAL_SETTING);
        digital_gain_setting.index_compensation = (uint32_t)((int32_t)((int16_t)gain2_compensation));
        digital_gain_setting.memory_select = HAL_AUDIO_MEMORY_DL_DL2|HAL_AUDIO_MEMORY_DL_SRC2;
        digital_gain_setting.sample_per_step = (uint32_t)gain2_per_step;
        hal_audio_control_set_value((hal_audio_set_value_parameter_t *)&digital_gain_setting, HAL_AUDIO_SET_VOLUME_HW_DIGITAL_SETTING);
        DSP_MW_LOG_I("gain1_compensation = 0x%x, gain2_compensation = 0x%x, gain1_per_step = 0x%x, gain2_per_step 0x%x", 4, gain1_compensation, gain2_compensation,gain1_per_step,gain2_per_step);
        #endif
    } else {
        int16_t gain3_compensation = (int16_t)((msg.ccni_message[1])&0xFFFF);
        uint16_t gain3_per_step = (uint16_t)((msg.ccni_message[0])&0xFF);
        #if 0//modify for ab1568
        hal_audio_set_gain_parameters(gain1_compensation, gain2_compensation, gain1_per_step, gain2_per_step);
        #else
        hal_audio_volume_digital_gain_setting_parameter_t   digital_gain_setting;
        digital_gain_setting.index_compensation = (uint32_t)((int32_t)((int16_t)gain3_compensation));
        digital_gain_setting.memory_select = HAL_AUDIO_MEMORY_DL_DL3;
        digital_gain_setting.sample_per_step = (uint32_t)gain3_per_step;
        hal_audio_control_set_value((hal_audio_set_value_parameter_t *)&digital_gain_setting, HAL_AUDIO_SET_VOLUME_HW_DIGITAL_SETTING);
        DSP_MW_LOG_I("gain3_compensation = 0x%x, gain3_per_step = 0x%x", 2, gain3_compensation, gain3_per_step);
        #endif
    }
    UNUSED(ack);
}


void DSP_GC_MuteInput(hal_ccni_message_t msg, hal_ccni_message_t *ack)
{
    hal_audio_stream_in_scenario_t type;
    uint16_t input_index = (uint16_t)(msg.ccni_message[0] & 0xFFFF);
    bool enable = (bool)(msg.ccni_message[1] & 0xFFFF);

    UNUSED(ack);

    type = (input_index >> 8) & 0x7F;
    if ((input_index & HAL_AUDIO_STREAM_IN_SCENARIO_MARK) && (type < HAL_AUDIO_STREAM_IN_SCENARIO_MAX)) {
#ifdef MTK_BT_HFP_ENABLE
#ifdef AIR_SOFTWARE_GAIN_ENABLE
        if ((type == HAL_AUDIO_STREAM_IN_SCENARIO_HFP)||(type == HAL_AUDIO_STREAM_IN_SCENARIO_BLE_CALL)) {
            DSP_MW_LOG_I("CALL UL_SW_Gain_Mute_control %d type %d", 2, enable,type);
            Call_UL_SW_Gain_Mute_control(enable);
            return;
        }
#endif
#endif
    }

    hal_audio_mute_stream_in(enable);
}

#ifdef USE_CCNI
void DSP_GC_SetHWGainWithFadeTime(hal_ccni_message_t msg, hal_ccni_message_t *ack)
{
    uint16_t output_index = (uint16_t)(msg.ccni_message[0]&0xFFFF);
    uint16_t d_gain_index = (uint16_t)msg.ccni_message[1];
    uint16_t fade_time = (uint32_t)(msg.ccni_message[1] >> 16);
    hal_audio_volume_digital_gain_fade_time_setting_parameter_t digital_gain_fade_time_setting;
    digital_gain_fade_time_setting.fade_time = fade_time;
    digital_gain_fade_time_setting.gain_index = (uint32_t)((int32_t)((int16_t)d_gain_index));

    if (output_index == 0) {
        //DL1
        digital_gain_fade_time_setting.memory_select = HAL_AUDIO_MEMORY_DL_DL1;
        hal_audio_control_set_value((hal_audio_set_value_parameter_t *)&digital_gain_fade_time_setting,HAL_AUDIO_SET_VOLUME_HW_DIGITAL_FADE_TIME_SETTING);


    } else if (output_index == 1) {
        //DL1
        digital_gain_fade_time_setting.memory_select = HAL_AUDIO_MEMORY_DL_DL2;
        hal_audio_control_set_value((hal_audio_set_value_parameter_t *)&digital_gain_fade_time_setting,HAL_AUDIO_SET_VOLUME_HW_DIGITAL_FADE_TIME_SETTING);
    }

    UNUSED(ack);
}
#endif

