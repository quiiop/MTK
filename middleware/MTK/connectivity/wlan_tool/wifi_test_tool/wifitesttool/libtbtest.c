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

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*
 * This is sample code for HE TX TB Tx test library implementation
 */
#if !defined(FREERTOS)
#include <sys/ioctl.h>
#include <arpa/inet.h>
#endif /*!defined(FREERTOS)*/
#include <stdio.h>
#include <stdint.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#if !defined(FREERTOS)
#include "iwlib.h"
#endif /*!defined(FREERTOS)*/
#include "libwifitest.h"
#include "libtbtest.h"
#if (CONFIG_WLAN_SERVICE_ENABLE == 1)
#include "net_adaption.h"
#endif /*CONFIG_WLAN_SERVICE_ENABLE*/


#if CONFIG_SUPPORT_FFT
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <regex.h>

#endif


/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
struct test_ru_info ru_info_list[MAX_MULTI_TX_STA];
struct test_ru_allocatoin ru_alloc;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
#if !defined(FREERTOS) /*freertos: redefined in wlan_service*/
static u_int8 test_he_bpscs[] = {
    1, 2, 2, 4, 4, 6, 6, 6, 8, 8, 10, 10    /* MCS0~11 */
};

static u_int8 test_he_rate_density[] = {
    2, 2, 4, 2, 4, 3, 4, 6, 4, 6, 4, 6  /* MCS0~11 */
};

static u_int8 test_ltf_sym[] = {
    0, 1, 2, 4, 4, 6, 6, 8, 8   /* SS 1~8 */
};

static u_int8 test_he_t_ltf_sym_x5[] = {
    24, 40, 80          /* 3.2+1.6 us, 6.4+1.6, 12.8+3.2 */
};

static u_int8 test_he_t_sym_x5[] = {
    68, 72, 80          /* base GI, double GI, quadruple GI */
};

static u_int8 test_he_t_pe_x5[] = {
    0, 20, 40, 60, 80       /* 0us, 4us, 8us, 12us, 16us */
};

static struct test_he_ru_const test_ru_const[] = {
    {37, 24, 12, 6, 2},
    {53, 48, 24, 12, 6},
    {61, 102, 51, 24, 12},
    {65, 234, 117, 60, 30},
    {67, 468, 234, 120, 60},
    {68, 980, 490, 240, 120},
    {69, 1960, 980, 492, 246}
};
#endif /*!defined(FREERTOS)*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
#if !defined(FREERTOS) /*freertos: redefined in wlan_service*/
static s_int32 mt_engine_map_subcarriers_short(
	u_int8 ru_index, u_int8 dcm);

static s_int32 mt_engine_map_subcarriers(
	u_int8 ru_index, u_int8 dcm);

static s_int32 mt_engine_calc_symbol_by_bytes(
	struct test_ru_info *ru_info, boolean stbc,
	u_char rate_den, u_int32 apep_length);


static s_int32 mt_engine_calc_afactor(struct test_ru_info *ru_info);

static s_int32 mt_engine_calc_pe_disamb(
	struct test_ru_info *ru_info, u_char ltf_gi, u_char max_pe);

static boolean mt_engine_calc_extr_sym(
	struct test_ru_info *ru_info, boolean stbc, u_char rd);

static s_int32 mt_engine_calc_l_ldpc(
	s_int32 avbits, s_int32 pld, u_char rate_den,
	s_int32 *cw, s_int32 *l_ldpc);

static s_int32 mt_engine_recalc_phy_info(
	struct test_ru_info *ru_info, u_int8 stbc, u_int8 ltf_gi, u_int8 max_pe);

void sys_ad_move_mem(void *dest, void *src, u_long length);
void sys_ad_zero_mem(void *ptr, u_long length);
void sys_ad_set_mem(void *ptr, u_long length, u_char value);
#endif /*!defined(FREERTOS)*/

static s_int32 get_param_and_shift_buf(
	boolean convert, u_int32 size, u_char **buf, u_char *out);

static s_int32 hqa_translate_ru_allocation(
	u_int32 user_ru_allocation,
	u_int32 *allocation);
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
#if !defined(FREERTOS) /*freertos: redefined in wlan_service*/
static s_int32 mt_engine_map_subcarriers(
	u_int8 ru_index, u_int8 dcm)
{
	s_int32 subcarriers = 0;
    u_int32 idx = 0;

	for (idx = 0 ; idx < SERV_ARRAY_SIZE(test_ru_const) ; idx++) {
		if (ru_index < test_ru_const[idx].max_index) {
			if (dcm)
				subcarriers = test_ru_const[idx].sd_d;
			else
				subcarriers = test_ru_const[idx].sd;

			break;
		}
	}

	return subcarriers;
}

static s_int32 mt_engine_calc_symbol_by_bytes(
	struct test_ru_info *ru_info, boolean stbc,
	u_char rate_den, u_int32 apep_length)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	s_int32 m_stbc = 1, tail = 6;
	u_int32 rate = 0;
	s_int32 ds = 0, dss = 0;

	ds = mt_engine_map_subcarriers(ru_info->ru_index >> 1,
					(ru_info->rate & BIT(5)));

	if (ds){
		dss = mt_engine_map_subcarriers_short(ru_info->ru_index >> 1,
						   (ru_info->rate & BIT(5)));
    }
	else
    {
#if DBG_TB
		printf("%s: unknown RU Index:[%d]!\n",
			__func__, (ru_info->ru_index >> 1));
#endif
		ret = SERV_STATUS_ENGINE_INVALID_PARAM;
		goto err_out;
	}

	rate = ru_info->rate & (~BIT(5));

	if (stbc)
		m_stbc++;

	if (ru_info->ldpc)
		tail = 0;

	ru_info->cbps = ds * ru_info->nss * test_he_bpscs[rate];
	ru_info->dbps = ru_info->cbps * (rate_den-1) / rate_den;
	ru_info->cbps_s = dss * ru_info->nss * test_he_bpscs[rate];
	ru_info->dbps_s = ru_info->cbps_s * (rate_den-1) / rate_den;

#if DBG_TB
    printf("%s: apep_length orig [%d]!\n",__func__, apep_length);
#endif

	apep_length *= 8;
	apep_length += (16 + tail);

	ru_info->symbol_init = engine_ceil(apep_length,
					(m_stbc * ru_info->dbps));
	ru_info->symbol_init *= m_stbc;
	ru_info->excess = (apep_length % (m_stbc * ru_info->dbps));

#if DBG_TB
	printf("%s: RU index[%d], apep length:%d symbol_init:%d,\n",
		__func__, ru_info->ru_index >> 1,
		apep_length, ru_info->symbol_init);
	printf("\t%s: R[%d/%d], cbps:%d, dbps:%d,\n",
		__func__, rate_den-1, rate_den, ru_info->cbps, ru_info->dbps);
	printf("\t%s: cbps_s:%d, dbps_s:%d excess:%d\n",
		__func__, ru_info->cbps_s,
		ru_info->dbps_s, ru_info->excess);
#endif

err_out:
	return ret;
}

static s_int32 mt_engine_map_subcarriers_short(
	u_int8 ru_index, u_int8 dcm)
{
	s_int32 subcarriers_short = 0;
    u_int32 idx = 0;

	for (idx = 0 ; idx < SERV_ARRAY_SIZE(test_ru_const) ; idx++) {
		if (ru_index < test_ru_const[idx].max_index) {
			if (dcm)
				subcarriers_short = test_ru_const[idx].sd_s_d;
			else
				subcarriers_short = test_ru_const[idx].sd_s;

			break;
		}
	}

	return subcarriers_short;
}
#endif /*!defined(FREERTOS)*/

s_int32 hqa_set_ru_info(
	u_char *data, u_int32 len, u_int32 tx_len, u_int32 band_idx)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	//u_int32 resp_len = 2, i;
	//u_int32 band_idx = (u_int32)(serv_test->ctrl_band_idx);
	u_int32 seg_sta_cnt[2] = {0}, sta_seq = 0, value = 0;
	u_char param_cnt = 0, segment_idx = 0, param_loop = 0;
	//u_char *data = hqa_frame->data;
	u_int32 mpdu_length = 0;
	struct test_ru_allocatoin *ru_allocation = NULL;
	struct test_ru_info *ru_info = NULL;


#if 0
    for (i=0; i<32; i++)
    {
        printf("%x ", data[i]);
    }
    printf("\n");
#endif

	get_param_and_shift_buf(true,
				   sizeof(u_int32),
				   &data,
				   (u_char *)&seg_sta_cnt[0]);

    //printf("seg_sta_cnt[0] %d", seg_sta_cnt[0]);

	get_param_and_shift_buf(true,
				   sizeof(u_int32),
				   &data,
				   (u_char *)&seg_sta_cnt[1]);

	//len -= sizeof(u_int32)*3;		/* array length */
	len -= sizeof(u_int32)*2;		/* paramter length, remove seg_sta_cnt[0]: 4 bytes /seg_sta_cnt[1]:4 bytes */

	if (seg_sta_cnt[0]+seg_sta_cnt[1] == 0) {
		ret = SERV_STATUS_AGENT_INVALID_LEN;
		goto err_out;
	}

	len /= (seg_sta_cnt[0]+seg_sta_cnt[1]);	/* per ru length */
	param_cnt = len/sizeof(u_int32);	/* param count */
	//printf("\n%s: Band:%d [ru_segment 0]:%d, \n[ru_segment 1]:%d\n",__func__, band_idx, seg_sta_cnt[0], seg_sta_cnt[1]);
	//printf("\t\tparameters count:%d\n", param_cnt);

	mpdu_length = tx_len;
	ru_allocation = &ru_alloc;
	ru_info = ru_info_list;
	sys_ad_zero_mem(ru_info, sizeof(struct test_ru_info)*MAX_MULTI_TX_STA);
	sys_ad_set_mem(ru_allocation, sizeof(*ru_allocation), 0xff);

	/* for maximum bw 80+80/160, 2 segments only */
	for (sta_seq = 0;
	     sta_seq < seg_sta_cnt[0]+seg_sta_cnt[1];
	     sta_seq++) {
		param_loop = param_cnt;

		if (sta_seq < seg_sta_cnt[0])
			segment_idx = 0;
		else
			segment_idx = 1;

		ru_info[sta_seq].valid = true;
		/* ru caterogy */
		get_param_and_shift_buf(true,
					   sizeof(u_int32),
					   &data,
					   (u_char *)&value);
		param_loop--;
		/* ru allocation */
		get_param_and_shift_buf(true,
					   sizeof(u_int32),
					   &data,
					   (u_char *)&value);
		param_loop--;
		hqa_translate_ru_allocation(value,
				    &ru_info[sta_seq].allocation);
		/* aid */
		get_param_and_shift_buf(true,
					   sizeof(u_int32),
					   &data,
					   (u_char *)&value);
		param_loop--;
		ru_info[sta_seq].aid = value;

        /* ru_index */
		get_param_and_shift_buf(true,
					   sizeof(u_int32),
					   &data,
					   (u_char *)&value);
		param_loop--;
		ru_info[sta_seq].ru_index = (value << 1) | segment_idx;

        /* MCS */
		get_param_and_shift_buf(true,
					   sizeof(u_int32),
					   &data,
					   (u_char *)&value);
		param_loop--;

		ru_info[sta_seq].rate = value;

        /* LDPC */
		get_param_and_shift_buf(true,
					   sizeof(u_int32),
					   &data,
					   (u_char *)&value);
		param_loop--;
		ru_info[sta_seq].ldpc = value;

        /* nss */
		get_param_and_shift_buf(true,
					   sizeof(u_int32),
					   &data,
					   (u_char *)&value);
		param_loop--;
		ru_info[sta_seq].nss = value;

        /* Stream0 */
		get_param_and_shift_buf(true,
					   sizeof(u_int32),
					   &data,
					   (u_char *)&value);
		param_loop--;
		ru_info[sta_seq].start_sp_st = value-1;

        /* mpdu_length */
		get_param_and_shift_buf(true,
					   sizeof(u_int32),
					   &data,
					   (u_char *)&value);
		param_loop--;
		if (value > 24)
			ru_info[sta_seq].mpdu_length = value;
		else
			ru_info[sta_seq].mpdu_length = mpdu_length;

#if DBG_TB
        printf("ru_info[%d].mpdu_length [%d]\n", sta_seq, ru_info[sta_seq].mpdu_length);
#endif

        /* alpha (tiger_debug: POWER ?)*/
		if (param_loop) {
			get_param_and_shift_buf(true,
						   sizeof(u_int32),
						   &data,
						   (u_char *)&value);
			param_loop--;
			ru_info[sta_seq].alpha = value;
		}

        /* ru_mu_nss */
		if (param_loop) {
			get_param_and_shift_buf(true,
						   sizeof(u_int32),
						   &data,
						   (u_char *)&value);
			param_loop--;
			ru_info[sta_seq].ru_mu_nss = value;
		}

#if 0
		printf(
			 "%s: ru_segment[%d][0x%x]: ru_idx:%d\n",
			 __func__, segment_idx,
			 ru_info[sta_seq].allocation,
			 ru_info[sta_seq].ru_index >> 1);
		printf(
			 "\t\t\t\trate:%x, ldpc:%d\n",
			 ru_info[sta_seq].rate,
			 ru_info[sta_seq].ldpc);
		printf(
			 "\t\t\t\tnss:%d, mimo nss:%d\n",
			 ru_info[sta_seq].nss,
			 ru_info[sta_seq].ru_mu_nss);
		printf(
			 "\t\t\t\t start spatial stream:%d,\n",
			 ru_info[sta_seq].start_sp_st);
		printf(
			 "\t\t\t\tmpdu length=%d, alpha:%d\n",
			 ru_info[sta_seq].mpdu_length,
			 ru_info[sta_seq].alpha);
#endif
	}


err_out:
	//update_hqa_frame(hqa_frame, resp_len, ret);

	return ret;
}

#if !defined(FREERTOS) /*freertos: redefined in wlan_service*/
s_int32 mt_engine_calc_phy(
	u_int8 stbc,
	u_int8 ltf_gi,
	u_int8 max_tpe)
{
	u_char rate_den = 0;
	struct test_ru_info *ru_info = &ru_info_list[0];
	u_int32 apep_length = ru_info->mpdu_length+13;

#if DBG_TB
    printf("%s: apep_length input [%d]!\n",__func__, apep_length);
#endif

	rate_den = test_he_rate_density[ru_info->rate & ~BIT(5)];
	mt_engine_calc_symbol_by_bytes(ru_info, stbc, rate_den, apep_length);
	mt_engine_calc_afactor(ru_info);
	mt_engine_calc_pe_disamb(ru_info, ltf_gi, max_tpe);

	if (ru_info->ldpc &&
		mt_engine_calc_extr_sym(ru_info, stbc, rate_den)) {
		ru_info->ldpc_extr_sym = 1;

		mt_engine_recalc_phy_info(ru_info, stbc, ltf_gi, max_tpe);
	}

	return SERV_STATUS_SUCCESS;
}

static s_int32 mt_engine_calc_afactor(struct test_ru_info *ru_info)
{
	s_int32 ret = 0, m_stbc = 1;

	if (ru_info->excess == 0) {
		ru_info->excess = m_stbc * ru_info->dbps;
		ru_info->afactor_init = 4;
	} else {
		u_int32 sym_short = (m_stbc * ru_info->dbps_s);
		u_int32 symbol = engine_ceil(ru_info->excess, sym_short);

		ru_info->afactor_init = engine_min(4, symbol);
	}

	/* prepare for caculate ldpc extra symbol */
	if (ru_info->afactor_init == 4) {
		ru_info->dbps_last = ru_info->dbps;
		ru_info->cbps_last = ru_info->cbps;
	} else {
		ru_info->dbps_last = ru_info->afactor_init * ru_info->dbps_s;
		ru_info->cbps_last = ru_info->afactor_init * ru_info->cbps_s;
	}

	ru_info->pld = (ru_info->symbol_init - m_stbc) * ru_info->dbps;
	ru_info->pld += m_stbc * ru_info->dbps_last;
	ru_info->avbits = (ru_info->symbol_init - m_stbc) * ru_info->cbps;
	ru_info->avbits += m_stbc * ru_info->cbps_last;

#if DBG_TB
	printf("\t%s: \tafactor=%d, symbol cnt=%d\n",
		__func__, ru_info->afactor_init, ru_info->symbol_init);
	printf("\t%s: cbps_l:%d, dbps_l:%d, pld:%d, avbits:%d\n",
		__func__, ru_info->cbps_last, ru_info->dbps_last,
		ru_info->pld, ru_info->avbits);
#endif
	return ret;
}

static s_int32 mt_engine_calc_pe_disamb(
	struct test_ru_info *ru_info, u_char ltf_gi, u_char max_pe)
{
	u_int8 pe_symbol_x5 = 0;
	s_int32 ret = 0, gi = 0;
	u_int32 t_pe = ru_info->afactor_init;
	s_int32 ltf_time = 0;
	u_int32 nss = engine_max(ru_info->ru_mu_nss, ru_info->nss);

	ltf_time = test_ltf_sym[nss];
	ltf_time *= test_he_t_ltf_sym_x5[ltf_gi];

	if (ltf_gi == 2)
		gi = TEST_GI_32;
	else
		gi = TEST_GI_16;

	/* txtime = 20 + T_HE-PREAMBLE + N_SYM*T_SYM +
	 *          N_MA*N_HE-LTF*T_HE-LTF-SYM + T_PE +
	 *          SignalExtension (28-135)
	 * T_HE-PREAMBLE = T_RL-SIG + T_HE-SIG-A +
	 *                 T_HE-STF-T + N_HE-LTF*T_HE-LTF-SYM,
	 *                 for an HE TB PPDU
	 * According to Table 28-12 of 802.11ax D3.0, T_RL-SIG = 4,
	 *                 T_HE-SIG-A = 8, T_HE-STF-T = 8,
	 *                 N_HE-LTF*T_HE-LTF-SYM (N_HE-LTF = {1,2,4,6}))
	 * N_MA = 0 due to doppler is not support, and SignalExtension = 0
	 *    due to not supported
	 */
	ru_info->tx_time_x5 = 5 * 20 + (20+40+40+ltf_time) +
			     ru_info->symbol_init * test_he_t_sym_x5[gi] + 0
			     + test_he_t_pe_x5[t_pe] + 0;
	ru_info->l_len = engine_ceil((ru_info->tx_time_x5-20*5), (4*5))*3-3-2;

	pe_symbol_x5 = test_he_t_pe_x5[t_pe];
	pe_symbol_x5 += (4 * (((ru_info->tx_time_x5 - 20 * 5)%20) ? 1 : 0));
	if (pe_symbol_x5 >= test_he_t_sym_x5[gi])
		ru_info->pe_disamb = 1;
	else
		ru_info->pe_disamb = 0;

	ru_info->t_pe = t_pe;


#if DBG_TB
    printf("\tltf_gi %d nss %d ltf_gi %d symbol_init %d t_pe %d ltf_time %d pe_symbol_x5 %d\n",
        ltf_gi, nss, ltf_gi, ru_info->symbol_init, ru_info->t_pe, ltf_time, pe_symbol_x5 );

	printf("\tL-Len=%d, PE Disambiguilty=%d\n",
		ru_info->l_len, ru_info->pe_disamb);

	printf("\t\ttx_time(x5)=%d, tx_ltf_sym(x5):%d,\n",
		ru_info->tx_time_x5, ltf_time);
	printf("\t\ttx_sym(x5):%d, tx_pe(x5):%d\n",
		test_he_t_sym_x5[gi], test_he_t_pe_x5[t_pe]);
#endif
	return ret;
}

static boolean mt_engine_calc_extr_sym(
	struct test_ru_info *ru_info, boolean stbc, u_char rd)
{
	boolean ret = false;
	s_int32 cw = 0, l_ldpc = 0, shrt = 0;

	mt_engine_calc_l_ldpc(ru_info->avbits, ru_info->pld, rd, &cw, &l_ldpc);

	shrt = (s_int32)((cw * l_ldpc * (rd-1) / rd) - ru_info->pld);
	if (shrt < 0)
		shrt = 0;
	ru_info->punc = (s_int16)(cw * l_ldpc - ru_info->avbits - shrt);
	if (ru_info->punc < 0)
		ru_info->punc = 0;
#if DBG_TB
	printf("\t%s: cw:%d, avbits:%d, punc:%d, l_ldpc:%d, shrt:%d\n",
		__func__, cw, ru_info->avbits, ru_info->punc, l_ldpc, shrt);
#endif
	if (((10 * ru_info->punc > cw * l_ldpc / rd) &&
		(5 * shrt < 6 * ru_info->punc * (rd-1))) ||
		(10 * ru_info->punc > 3 * cw * l_ldpc / rd))
		ret = true;

#if DBG_TB
	printf("\tLDPC extra symbol:%d\n", ret);
#endif

	return ret;
}

static s_int32 mt_engine_calc_l_ldpc(
	s_int32 avbits, s_int32 pld, u_char rate_den,
	s_int32 *cw, s_int32 *l_ldpc)
{
	if (avbits <= 648) {
		*cw = 1;
		*l_ldpc = ((avbits >= (pld + 912/rate_den)) ? 2 : 1) * 648;
	} else if (avbits <= (648 * 2)) {
		*cw = 1;
		*l_ldpc = ((avbits >= (pld + 1464/rate_den)) ? 3 : 2) * 648;
	} else if (avbits <= (648 * 3)) {
		*cw = 1;
		*l_ldpc = (648 * 3);
	} else if (avbits <= (648 * 4)) {
		*cw = 2;
		*l_ldpc = ((avbits >= (pld + 2916/rate_den)) ? 3 : 2) * 648;
	} else {
		*cw = engine_ceil((pld * rate_den), ((648 * 3) * (rate_den-1)));
		*l_ldpc = (648 * 3);
	}

	return 0;
}

static s_int32 mt_engine_recalc_phy_info(
	struct test_ru_info *ru_info, u_int8 stbc, u_int8 ltf_gi, u_int8 max_pe)
{
	u_char rd = 0;
	u_char m_stbc = (stbc) ? 2 : 1;
	s_int32 shrt = 0;
	u_int32 cw = 0, l_ldpc = 0;

	rd = test_he_rate_density[ru_info->rate & ~BIT(5)];

	if (ru_info->afactor_init == 3) {
		u_int32 short_sym = ru_info->afactor_init * ru_info->cbps_s;

		ru_info->avbits += m_stbc * (ru_info->cbps - short_sym);
	} else
		ru_info->avbits += m_stbc * ru_info->cbps_s;

	mt_engine_calc_l_ldpc(ru_info->avbits, ru_info->pld,
			      rd, (s_int32 *)&cw, (s_int32 *)&l_ldpc);

	shrt = (s_int32)(cw * l_ldpc * (rd-1) / rd - ru_info->pld);
	if (shrt < 0)
		shrt = 0;
	ru_info->punc = (s_int32)(cw * l_ldpc - ru_info->avbits - shrt);
	if (ru_info->punc < 0)
		ru_info->punc = 0;

	if (ru_info->afactor_init == 4) {
		ru_info->symbol_init += m_stbc;
		ru_info->afactor_init = 1;
	} else
		ru_info->afactor_init++;

#if DBG_TB
	printf("\t\t%s: (re)afactor:%d\n",
		__func__, ru_info->afactor_init);
	printf("\t\t%s: (re)cw:%d, (re)avbits:%d,\n",
		__func__, cw, ru_info->avbits);
	printf("\t\t%s: (re)punc:%d, (re)l_ldpc:%d, (re)shrt:%d\n",
		__func__, ru_info->punc, l_ldpc, shrt);
#endif
	mt_engine_calc_pe_disamb(ru_info, ltf_gi, max_pe);

	return SERV_STATUS_SUCCESS;
}

void sys_ad_move_mem(void *dest, void *src, u_long length)
{
	memmove(dest, src, length);
}

void sys_ad_zero_mem(void *ptr, u_long length)
{
	memset(ptr, 0, length);
}

void sys_ad_set_mem(void *ptr, u_long length, u_char value)
{
	memset(ptr, value, length);
}
#endif /*!defined(FREERTOS)*/

static s_int32 get_param_and_shift_buf(
	boolean convert, u_int32 size, u_char **buf, u_char *out)
{
	if (!(*buf)) {
		printf("*buf NULL pointer with size=%u\n", size);
		return SERV_STATUS_AGENT_INVALID_NULL_POINTER;
	}

	if (!out) {
		printf("out NULL pointer with size=%u\n", size);
		return SERV_STATUS_AGENT_INVALID_NULL_POINTER;
	}

	sys_ad_move_mem(out, *buf, size);
	*buf = *buf + size;

	if (!convert) {
		//printf("%s: size=%u", __func__, size);
		return SERV_STATUS_SUCCESS;
	}

	if (size == sizeof(u_int32)) {
		u_int32 *tmp = (u_int32 *) out;

		*tmp = SERV_OS_NTOHL(*tmp);
		//printf("%s: size=%u, val=%u\n", __func__, size, *tmp);
	} else if (size == sizeof(u_int16)) {
		u_int16 *tmp = (u_int16 *) out;

		*tmp = SERV_OS_NTOHS(*tmp);
		//printf("%s: size=%u, val=%u\n", __func__, size, *tmp);
	} else {
		//printf("%s: size %u not supported\n", __func__, size);
		return SERV_STATUS_AGENT_NOT_SUPPORTED;
	}

	return SERV_STATUS_SUCCESS;
}

static s_int32 hqa_translate_ru_allocation(
	u_int32 user_ru_allocation,
	u_int32 *allocation)
{
	u_int8 i = 0;

	*allocation = 0;
	for (i = 0 ; i < sizeof(u_int32)*2 ; i++) {
		*allocation |= ((user_ru_allocation & 0x1) << i);
		user_ru_allocation >>= 4;
	}

	return SERV_STATUS_SUCCESS;
}

void mt_op_set_manual_he_tb_value(
    ENUM_WIFI_TEST_GI_TYPE sgi,
    uint32_t stbc,
    uint32_t bw)
{
	union hetb_rx_cmm cmm;
	union hetb_tx_usr usr;
	u_int8 ltf_sym_code[] = {
		0, 0, 1, 2, 2, 3, 3, 4, 4   /* SS 1~8 */
	};

    struct test_ru_info *ru_sta = &ru_info_list[0];

	/* setup MAC start */
	/* step 1, common info of TF */
	sys_ad_zero_mem(&cmm, sizeof(cmm));
	cmm.field.sig_a_reserved = 0x1ff;
	cmm.field.ul_length = ru_sta->l_len;
	cmm.field.t_pe =
	(ru_sta->afactor_init & 0x3) | ((ru_sta->pe_disamb & 0x1) << 2);
	cmm.field.ldpc_extra_sym = ru_sta->ldpc_extr_sym;
	if (stbc && ru_sta->nss == 1)
		cmm.field.ltf_sym_midiam = ltf_sym_code[ru_sta->nss+1];
	else
		cmm.field.ltf_sym_midiam = ltf_sym_code[ru_sta->nss];
	cmm.field.gi_ltf = sgi;
	cmm.field.ul_bw = bw;
	cmm.field.stbc = stbc;

#if DBG_TB
	printf("%s: [TF TTRCR0 ] tigger_type:0x%x,\n",
	__func__, cmm.field.tigger_type);
	printf("ul_length:0x%x cascade_ind:0x%x,\n",
	cmm.field.ul_length, cmm.field.cascade_ind);

	printf("cs_required:0x%x, ul_bw:0x%x,\n",
	cmm.field.cs_required, cmm.field.ul_bw);

	printf("gi_ltf:0x%x, mimo_ltf:0x%x,\n",
	cmm.field.gi_ltf, cmm.field.mimo_ltf);

	printf("ltf_sym_midiam:0x%x, stbc:0x%x,\n",
	cmm.field.ltf_sym_midiam, cmm.field.stbc);

	printf("ldpc_extra_sym :0x%x, ap_tx_pwr: 0x%x\n",
	cmm.field.ldpc_extra_sym, cmm.field.ap_tx_pwr);

	printf("%s: [TF TTRCR0 ] cr_value:0x%08x\n",
	__func__, (u_int32)(cmm.cmm_info & 0xffffffff));

	printf("%s: [TF TTRCR1 ] cr_value:0x%08x\n",
	__func__, (u_int32)((cmm.cmm_info & 0xffffffff00000000) >> 32));
#endif

	/* step 1, users info */
	sys_ad_zero_mem(&usr, sizeof(usr));
	usr.field.aid = 0x1;
	usr.field.allocation = ru_sta->ru_index;
	usr.field.coding = ru_sta->ldpc;
	usr.field.mcs = ru_sta->rate & ~BIT(5);
	usr.field.dcm = (ru_sta->rate & BIT(5)) >> 4;
	usr.field.ss_allocation =
	((ru_sta->nss-1) << 3) | (ru_sta->start_sp_st & 0x7);


#if DBG_TB
	printf("%s: [TF TTRCR2 ] aid:%d\n",	__func__,  usr.field.aid);
	printf("allocation:%d, coding:%d,\n",usr.field.allocation, usr.field.coding);
	printf("mcs:0x%x, dcm:%d,\n", usr.field.mcs, usr.field.dcm);
	printf("ss_allocation:%d\n",usr.field.ss_allocation);
	printf("%s: [TF TTRCR2 ] cr_value:0x%08x\n",__func__, usr.usr_info);
#endif

	WIFI_TEST_CONFIG_TTRCR(RF_AT_FUNCID_SET_TX_HE_TB_TTRCR0, (cmm.cmm_info & 0xffffffff));

    WIFI_TEST_CONFIG_TTRCR(RF_AT_FUNCID_SET_TX_HE_TB_TTRCR1,(cmm.cmm_info & 0xffffffff00000000) >> 32);

	WIFI_TEST_CONFIG_TTRCR(RF_AT_FUNCID_SET_TX_HE_TB_TTRCR2, usr.usr_info);

	WIFI_TEST_CONFIG_TTRCR(RF_AT_FUNCID_SET_TX_HE_TB_TTRCR3, 0x7f);

	WIFI_TEST_CONFIG_TTRCR(RF_AT_FUNCID_SET_TX_HE_TB_TTRCR4, 0xffffffff);

	WIFI_TEST_CONFIG_TTRCR(RF_AT_FUNCID_SET_TX_HE_TB_TTRCR5, 0xffffffff);

	WIFI_TEST_CONFIG_TTRCR(RF_AT_FUNCID_SET_TX_HE_TB_TTRCR6, 0xffffffff);

    mt_engine_dump_ruinfo();

}

void mt_engine_dump_ruinfo(void)
{
    struct test_ru_info *ru_sta = &ru_info_list[0];
    printf("RU Setting Dump ---\n");
    //printf("\naid  = %d", ru_sta->aid);
    printf("\nldpc_extr_sym  = %d", ru_sta->ldpc_extr_sym);
    printf("\npe_disamb  = %d", ru_sta->pe_disamb);
    printf("\nafactor  = %d", ru_sta->afactor_init);
    printf("\nUser  = %d", ru_sta->aid);
    printf("\nnss  = %d", ru_sta->nss);
    printf("\n# Stream  = %d", (ru_sta->start_sp_st+1));
    printf("\nMCS  = %d", ru_sta->rate);
    printf("\nRU idx  = %d", (ru_sta->ru_index >> 1));
    printf("\nLDPC  = %d\n", ru_sta->ldpc);

}

u_int32 mt_engine_get_ru_mpdu_length(void)
{
    struct test_ru_info *ru_sta = &ru_info_list[0];

    return ru_sta->mpdu_length;
}

u_int32 mt_engine_get_ru_ldpc(void)
{
    struct test_ru_info *ru_sta = &ru_info_list[0];

    return ru_sta->ldpc;
}


u_int32 mt_engine_get_ru_mcs_rate(void)
{
    struct test_ru_info *ru_sta = &ru_info_list[0];

    return ru_sta->rate;
}




