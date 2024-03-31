/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/


/*
 * This is sample code for WLAN Test Mode Control
 */

#ifndef __LIBTBTEST_H__
#define __LIBTBTEST_H__


/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/
#define DBG_TB 0

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define SERV_IOCTLBUFF 2048
#define MAX_MULTI_TX_STA 2

#if !defined(FREERTOS) /*freertos: redefined in wlan_service*/
#define SERV_STATUS_SUCCESS				0x0000

/* Agent module: failure */
#define SERV_STATUS_AGENT_FAIL				0x0100
/* Agent module: invalid null pointer */
#define SERV_STATUS_AGENT_INVALID_NULL_POINTER		0x0101
/* Agent module: invalid band idx */
#define SERV_STATUS_AGENT_INVALID_BANDIDX		0x0102
/* Agent module: invalid length */
#define SERV_STATUS_AGENT_INVALID_LEN			0x0103
/* Agent module: invalid parameter */
#define SERV_STATUS_AGENT_INVALID_PARAM			0x0104
/* Agent module: not supported */
#define SERV_STATUS_AGENT_NOT_SUPPORTED			0x0105

/* Service test module: failure */
#define SERV_STATUS_SERV_TEST_FAIL			0x0200
/* Service test module: invalid null pointer */
#define SERV_STATUS_SERV_TEST_INVALID_NULL_POINTER	0x0201
/* Service test module: invalid band idx */
#define SERV_STATUS_SERV_TEST_INVALID_BANDIDX		0x0202
/* Service test module: invalid length */
#define SERV_STATUS_SERV_TEST_INVALID_LEN		0x0203
/* Service test module: invalid parameter */
#define SERV_STATUS_SERV_TEST_INVALID_PARAM		0x0204
/* Service test module: not supported */
#define SERV_STATUS_SERV_TEST_NOT_SUPPORTED		0x0205

/* Test engine module: failure */
#define SERV_STATUS_ENGINE_FAIL				0x0300
/* Test engine module: invalid null pointer */
#define SERV_STATUS_ENGINE_INVALID_NULL_POINTER		0x0301
/* Test engine module: invalid band idx */
#define SERV_STATUS_ENGINE_INVALID_BANDIDX		0x0302
/* Test engine module: invalid length */
#define SERV_STATUS_ENGINE_INVALID_LEN			0x0303
/* Test engine module: invalid parameter */
#define SERV_STATUS_ENGINE_INVALID_PARAM		0x0304
/* Test engine module: not supported */
#define SERV_STATUS_ENGINE_NOT_SUPPORTED		0x0305

/* Hal test mac module: failure */
#define SERV_STATUS_HAL_MAC_FAIL			0x0400
/* Hal test mac module: invalid padapter */
#define SERV_STATUS_HAL_MAC_INVALID_PAD			0x0401
/* Hal test mac module: invalid null pointer */
#define SERV_STATUS_HAL_MAC_INVALID_NULL_POINTER	0x0402
/* Hal test mac module: invalid band idx */
#define SERV_STATUS_HAL_MAC_INVALID_BANDIDX		0x0403
/* Hal test mac module: un-registered chip ops */
#define SERV_STATUS_HAL_MAC_INVALID_CHIPOPS		0x0404

/* Hal operation module: failure */
#define SERV_STATUS_HAL_OP_FAIL				0x0500
/* Hal operation module: failure to send fw command */
#define SERV_STATUS_HAL_OP_FAIL_SEND_FWCMD		0x0501
/* Hal operation module: failure to set mac behavior */
#define SERV_STATUS_HAL_OP_FAIL_SET_MAC			0x0502
/* Hal operation module: invalid padapter */
#define SERV_STATUS_HAL_OP_INVALID_PAD			0x0503
/* Hal operation module: invalid null pointer */
#define SERV_STATUS_HAL_OP_INVALID_NULL_POINTER		0x0504
/* Hal operation module: invalid band idx */
#define SERV_STATUS_HAL_OP_INVALID_BANDIDX		0x0505

/* Osal net adaption module: failure */
#define SERV_STATUS_OSAL_NET_FAIL			0x0600
/* Osal net adaption module: failure to send fw command */
#define SERV_STATUS_OSAL_NET_FAIL_SEND_FWCMD		0x0601
/* Osal net adaption module: failure to init wdev */
#define SERV_STATUS_OSAL_NET_FAIL_INIT_WDEV		0x0602
/* Osal net adaption module: failure to release wdev */
#define SERV_STATUS_OSAL_NET_FAIL_RELEASE_WDEV		0x0603
/* Osal net adaption module: failure to update wdev */
#define SERV_STATUS_OSAL_NET_FAIL_UPDATE_WDEV		0x0604
/* Osal net adaption module: failure to set channel */
#define SERV_STATUS_OSAL_NET_FAIL_SET_CHANNEL		0x0605
/* Osal net adaption module: invalid padapter */
#define SERV_STATUS_OSAL_NET_INVALID_PAD		0x0606
/* Osal net adaption module: invalid null pointer */
#define SERV_STATUS_OSAL_NET_INVALID_NULL_POINTER	0x0607
/* Osal net adaption module: invalid band idx */
#define SERV_STATUS_OSAL_NET_INVALID_BANDIDX		0x0608
/* Osal net adaption module: invalid length */
#define SERV_STATUS_OSAL_NET_INVALID_LEN		0x0609
/* Osal net adaption module: invalid parameter */
#define SERV_STATUS_OSAL_NET_INVALID_PARAM		0x060A

/* Osal sys adaption module: failure */
#define SERV_STATUS_OSAL_SYS_FAIL			0x0700
/* Osal sys adaption module: invalid padapter */
#define SERV_STATUS_OSAL_SYS_INVALID_PAD		0x0701
/* Osal sys adaption module: invalid null pointer */
#define SERV_STATUS_OSAL_SYS_INVALID_NULL_POINTER	0x0702
/* Osal sys adaption module: invalid band idx */
#define SERV_STATUS_OSAL_SYS_INVALID_BANDIDX		0x0703
#endif /*!defined(FREERTOS)*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
typedef signed char s_int8;
typedef signed short s_int16;
typedef signed int s_int32;
typedef signed long long s_int64;
typedef unsigned char u_int8;
typedef unsigned short u_int16;
typedef unsigned int u_int32;
typedef unsigned long long u_int64;

typedef signed char s_char;
typedef signed short s_short;
typedef signed long s_long;
typedef signed long long s_longlong;
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
typedef unsigned long long u_longlong;

typedef signed int *ps_int32;
typedef signed long long *ps_int64;
typedef unsigned char *pu_int8;
typedef unsigned short *pu_int16;
typedef unsigned int *pu_int32;
typedef unsigned long long *pu_int64;

typedef s_char *ps_char;
typedef u_char *pu_char;
typedef u_short *pu_short;
typedef s_long *ps_long;
typedef u_long *pu_long;

typedef unsigned char boolean;

#if (CONFIG_WLAN_SERVICE_ENABLE == 1)
extern struct test_ru_info ru_info_list[MAX_MULTI_TX_STA];
#endif /*CONFIG_WLAN_SERVICE_ENABLE*/

#if !defined(FREERTOS) /*freertos: redefined in wlan_service*/
/* Test HE GI definition */
enum test_he_gi_type {
	TEST_GI_8,
	TEST_GI_16,
	TEST_GI_32,
};

struct test_ru_allocatoin {
	/* maximum 8 sub-20MHz for 160/80+80 MHz bandwidth */
	u_int8 sub20[8];
};

struct test_ru_info {
	boolean valid;
	u_int32 aid;
	u_int32 allocation;
	u_int32 ru_index;
	u_int32 rate;
	u_int32 ldpc;
	u_int32 nss;
	u_int32 start_sp_st;
	u_int32 mpdu_length;
	s_int32 alpha;
	u_int32 ru_mu_nss;
	/* end of user input*/
	u_int32 t_pe;
	u_int32 afactor_init;   /* A Factor */
	u_int32 symbol_init;
	u_int32 excess;
	u_int32 dbps;
	u_int32 cbps;
	u_int32 dbps_s;
	u_int32 cbps_s;
	u_int32 pld;
	u_int32 avbits;
	u_int32 dbps_last;
	u_int32 cbps_last;
	u_int8 ldpc_extr_sym;   /* LDPC sym.*/
	u_int32 tx_time_x5;
	u_int8 pe_disamb;       /* PE Disamb. */
	s_int16 punc;
	u_int32 l_len;
};

struct  hqa_frame {
	u_int32 magic_no;
	u_int16 type;
	u_int16 id;
	u_int16 length;
	u_int16 sequence;
	u_int8 data[SERV_IOCTLBUFF];
};
#endif /*!defined(FREERTOS)*/

union hetb_rx_cmm {
	struct {
		unsigned long long tigger_type:4;
		unsigned long long ul_length:12;
		unsigned long long cascade_ind:1;
		unsigned long long cs_required:1;
		unsigned long long ul_bw:2;
		unsigned long long gi_ltf:2;
		unsigned long long mimo_ltf:1;
		unsigned long long ltf_sym_midiam:3;
		unsigned long long stbc:1;
		unsigned long long ldpc_extra_sym:1;
		unsigned long long ap_tx_pwr:6;
		unsigned long long t_pe:3;
		unsigned long long spt_reuse:16;
		unsigned long long doppler:1;
		unsigned long long sig_a_reserved:9;
		unsigned long long reserved:1;
	} field;
	unsigned long long cmm_info;
};

union hetb_tx_usr {
	struct {
		u_int32 aid:12;
		u_int32 allocation:8;
		u_int32 coding:1;
		u_int32 mcs:4;
		u_int32 dcm:1;
		u_int32 ss_allocation:6;
	} field;
	u_int32 usr_info;
};

#if !defined(FREERTOS) /*freertos: redefined in wlan_service*/
#if 1
/* Service data struct for test mode usage */
struct service_test {
	/*========== Jedi only ==========*/
	/* Wlan related information which test needs */
	//struct test_wlan_info *test_winfo;

	/* Test backup CR */
	//struct test_bk_cr test_bkcr[TEST_MAX_BKCR_NUM];

	/* Test Rx statistic */
	//struct test_rx_stat test_rx_statistic[TEST_DBDC_BAND_NUM];

	/* The band related state which communicate between UI/driver */
	//struct test_band_state test_bstat;

	/* The band_idx which user wants to control currently */
	u_char ctrl_band_idx;

	//struct test_backup_params test_backup;

	/*========== Common part ==========*/
	/* Test configuration */
	//struct test_configuration test_config[TEST_DBDC_BAND_NUM];

	/* Test operation */
	//struct test_operation *test_op;

	/* Test control register read/write */
	//struct test_register test_reg;

	/* Test eeprom read/write */
	//struct test_eeprom test_eprm;

	/* Test tmr related configuration */
	//struct test_tmr_info test_tmr;

	/* Jedi: false, Gen4m: true */
	boolean engine_offload;

	/* TODO: factor out here for log dump */
	u_int32 en_log;
	//struct test_log_dump_cb test_log_dump[TEST_LOG_TYPE_NUM];
};
#endif


struct test_he_ru_const {
	u_int8	max_index;
	u_int16	sd;		/* data subcarriers */
	u_int16	sd_d;		/* data subcarriers for DCM */
	u_int16	sd_s;		/* data subcarriers short */
	u_int16	sd_s_d;		/* data subcarriers short for DCM*/
};
#endif /*!defined(FREERTOS)*/



/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/* Array size */
#define SERV_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#if !defined(FREERTOS) /*freertos: redefined in wlan_service*/
#define SERV_LOG(category, level, fmt)
#endif

#ifndef BIT
#define BIT(n)                          ((uint32_t) 1UL << (n))
#endif /* BIT */

#define engine_min(_a, _b) ((_a > _b) ? _b : _a)

#define engine_max(_a, _b) ((_a > _b) ? _a : _b)

#define engine_ceil(_a, _b) (((_a%_b) > 0) ? ((_a/_b)+1) : (_a/_b))

#define TEST_ANT_USER_DEF 0x80000000

/* NTOHS/NTONS/NTOHL/NTONS */
#define SERV_OS_NTOHS(_val)	(ntohs((_val)))
#define SERV_OS_HTONS(_val)	(htons((_val)))
#define SERV_OS_NTOHL(_val)	(ntohl((_val)))
#define SERV_OS_HTONL(_val)	(htonl((_val)))


/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

s_int32 hqa_set_ru_info(
	u_char *data, u_int32 len, u_int32 tx_len, u_int32 band_idx);

u_int32 mt_engine_get_ru_mpdu_length(void);
u_int32 mt_engine_get_ru_ldpc(void);
u_int32 mt_engine_get_ru_mcs_rate(void);


#if (CONFIG_WLAN_SERVICE_ENABLE == 1)
extern s_int32 mt_engine_calc_phy(
	struct test_ru_info *ru_info,
	u_int32 apep_length,
	u_int8 stbc,
	u_int8 ltf_gi,
	u_int8 max_tpe);
#else /*CONFIG_WLAN_SERVICE_ENABLE*/
s_int32 mt_engine_calc_phy(
	u_int8 stbc,
	u_int8 ltf_gi,
	u_int8 max_tpe);
#endif

void mt_op_set_manual_he_tb_value(
    ENUM_WIFI_TEST_GI_TYPE sgi,
    uint32_t stbc,
    uint32_t bw);

void mt_engine_dump_ruinfo(void);



#endif /* __LIBTBTEST_H__ */
