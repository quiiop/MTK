#include "effp_test.h"
#include "va_process.h"
#include "preproc_process.h"
#include "EFFP_Memory.h"
#include "EFFP_Interface.h"
#include "mtk_heap.h"
#include "hw_res_mgr_implement.h"

#ifdef TEST_PEFMON
#include <xtensa/xt_perfmon.h>
#endif

#define __ram0_mem_size__ (344832)
#define __ram1_mem_size__ (16384)

extern int holiday_model[];
extern int model_size;

extern unsigned int test_pattern_in[];
extern unsigned int test_pattern_out[];
extern unsigned int test_pattern[];
extern int pattern_size;
extern int dsp_clk_res_ops_set(int value);

static int realized = 0;
struct ffp_mem_setting *mem_setting;
static int g_beam;
struct mcps_calc ffp_mcps;

static int effp_init(void)
{
    void *sram_ptr, *tcm_ptr;
    EFFP_MemBlock mic_config;
    EFFP_MemConfig mem_config;
    int require_sram_size, require_tcm_size;
    EFFP_MemSpecifier require_mem, supplyMem;
    int ret = 0;

    dsp_clk_res_ops_set(DSP_CLK_600M);

    mem_setting =
        (struct ffp_mem_setting *)(MTK_pvPortMalloc(sizeof(struct ffp_mem_setting),
            get_adsp_heap_type(ADSP_MEM_LP_CACHE)));
    configASSERT(mem_setting != NULL);
    memset(mem_setting, 0, sizeof(struct ffp_mem_setting));

    sram_ptr = tcm_ptr = NULL;

    printf("EFFP version: %s\n", EFFP_version());
    if (!realized) {
        if (!EFFP_realize())
            PRINTF_E("%s::EFFP_realize fail, err=%d\n", __func__, EFFP_getErrorCode());
	else
	    realized = 1;
    }

    EFFP_MemBlock_reset(&mic_config);
    mic_config.ptr = (void*)holiday_model;
    mic_config.byteNum = model_size * sizeof(int);

    ret = EFFP_prepare(2, 2, mic_config, 160, 2);
    if (ret == 0) {
        PRINTF_E("%s::EFFP_prepare fail, err=%d\n", __func__, EFFP_getErrorCode());
        MTK_vPortFree(mem_setting);
        mem_setting = NULL;
        return -1;
    }

    PRINTF_I("effp_value_id_input_signals_byte_num = %d\n",
             EFFP_getValue_int(effp_value_id_input_signals_byte_num));
    PRINTF_I("effp_value_id_output_signals_byte_num = %d\n",
             EFFP_getValue_int(effp_value_id_output_signals_byte_num));

    EFFP_MemSpecifier_reset(&supplyMem);
    EFFP_MemSpecifier_reset(&require_mem);
    EFFP_MemSpecifier_setAdequate(&supplyMem, 0);
    EFFP_MemSpecifier_setSize(&supplyMem, 1, __ram1_mem_size__);

    if (EFFP_queryMem(&supplyMem, &require_mem) == 0) {
        PRINTF_E("%s::EFFP_queryMem fail, err=%d\n", __func__, EFFP_getErrorCode());
        EFFP_conclude();
        MTK_vPortFree(mem_setting);
        mem_setting = NULL;
        return -1;
    }

    require_sram_size = EFFP_MemSpecifier_getSize(&require_mem, 0);
    require_tcm_size = EFFP_MemSpecifier_getSize(&require_mem, 1);

    if (require_sram_size) {
        mem_setting->sram_addr =
            MTK_pvPortMalloc(__ram0_mem_size__ + 8, get_adsp_heap_type(ADSP_MEM_NORMAL_CACHE));
        sram_ptr = (void*)(0x8 - ((TickType_t)(mem_setting->sram_addr) & 0x7) + (TickType_t)mem_setting->sram_addr);
        PRINTF_I("sram addr:%p<-->%p\n", mem_setting->sram_addr, sram_ptr);
    }
    PRINTF_I("require_sram_size:%d\n", require_sram_size);

    if (require_tcm_size) {
        mem_setting->tcm_addr =
            MTK_pvPortMalloc(__ram1_mem_size__ + 8, get_adsp_heap_type(ADSP_MEM_LP_CACHE));

        tcm_ptr = (void*)(0x8 - ((TickType_t)(mem_setting->tcm_addr) & 0x7) + (TickType_t)mem_setting->tcm_addr);
        PRINTF_I("tcmMem addr:%p<-->%p\n", mem_setting->tcm_addr, tcm_ptr);
    }
    PRINTF_I("require_tcm_size:%d\n", require_tcm_size);

    EFFP_MemConfig_set(&mem_config,
                       0,
                       sram_ptr,
                       require_sram_size);
    EFFP_MemConfig_set(&mem_config,
                       1,
                       tcm_ptr,
                       require_tcm_size);

    if (EFFP_dispatchMem(&mem_config) == 0) {
        PRINTF_E("%s::EFFP_dispatchMem fail, err=%d\n", __func__, EFFP_getErrorCode());
        EFFP_conclude();
        MTK_vPortFree(mem_setting->tcm_addr);
        MTK_vPortFree(mem_setting->sram_addr);
        MTK_vPortFree(mem_setting);
        mem_setting = NULL;
        return -1;
    }

    if (EFFP_resetStream() == 0) {
        PRINTF_E("%s::EFFP_resetStream fail, err=%d\n", __func__, EFFP_getErrorCode());
        EFFP_conclude();
        MTK_vPortFree(mem_setting->tcm_addr);
        MTK_vPortFree(mem_setting->sram_addr);
        MTK_vPortFree(mem_setting);
        mem_setting = NULL;
        return -1;
    }

    return ret;
}


static int effp_uninit(void)
{
    if (EFFP_conclude() == 0) {
        PRINTF_E("EFFP_conclude fail, err=%d\n", EFFP_getErrorCode());
    }
    if (mem_setting && mem_setting->tcm_addr) {
        MTK_vPortFree(mem_setting->tcm_addr);
        mem_setting->tcm_addr= NULL;
    }
    if (mem_setting && mem_setting->sram_addr) {
        MTK_vPortFree(mem_setting->sram_addr);
        mem_setting->sram_addr = NULL;
    }
    if (mem_setting) {
        MTK_vPortFree(mem_setting);
        mem_setting = NULL;
    }
    return 0;
}

static int effp_process(unsigned int *input, unsigned int *output)
{
    int ret = 0;
    EFFP_MemBlock input_signals, output_signals;

    input_signals.ptr = (void*)input;
    input_signals.byteNum = 32 * 4 * 160 / 8;
    output_signals.ptr = (void*)output;
    output_signals.byteNum = 32 * 2 * 160 / 8;
    ret = EFFP_processStream(input_signals, output_signals, &(g_beam));
    if (ret == 0) {
        PRINTF_E("EFFP_processStream fail, err=%d\n", EFFP_getErrorCode());
    }

    return ret;
}

static int cli_cmd_effp_test(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    int i = 0;
    unsigned int *inp;
    unsigned int *outp;
#ifdef TEST_PEFMON
    counter_id_t ic1, ic2;
    counter_id_t dc1, dc2, dc3, dc4;
    unsigned int iv1, iv2;
    unsigned int dv1, dv2, dv3, dv4;
    int cnums;

    xt_perf_clear();
    cnums = xt_perf_counters_num();
    PRINTF_E("[XtPerf] cnums = %d\n", cnums);

    ic1 = xt_perf_init_counter32(XTPERF_CNT_I_MEM, XTPERF_MASK_I_MEM_CACHE_HITS, 0);
    ic2 = xt_perf_init_counter32(XTPERF_CNT_I_MEM, XTPERF_MASK_I_MEM_CACHE_MISSES, 0);

    dc1 = xt_perf_init_counter32(XTPERF_CNT_D_LOAD_U1, XTPERF_MASK_D_LOAD_CACHE_HITS, 0);
    dc2 = xt_perf_init_counter32(XTPERF_CNT_D_LOAD_U1, XTPERF_MASK_D_LOAD_CACHE_MISSES, 0);
    dc3 = xt_perf_init_counter32(XTPERF_CNT_D_STORE_U1, XTPERF_MASK_D_STORE_CACHE_HITS, 0);
    dc4 = xt_perf_init_counter32(XTPERF_CNT_D_STORE_U1, XTPERF_MASK_D_STORE_CACHE_MISSES, 0);

    xt_perf_enable();
#endif

    effp_init();

    for (i = 0; i < 8000; i++) {
        test_pattern_in[i*4] = test_pattern[i];
        test_pattern_in[i*4+1] = test_pattern[i];
        test_pattern_in[i*4+2] = test_pattern[i];
        test_pattern_in[i*4+3] = test_pattern[i];
    }
    PRINTF_E("[%s] test start.\n", __func__);

    for (i = 0; i < 8000/160; i++) {
        inp = test_pattern_in + i*160*4;
        outp = test_pattern_out + i*160*2;

        mcps_calc_before_proc(&ffp_mcps);
#ifdef TEST_PEFMON
        xt_perf_reset_counter(ic1);
        xt_perf_reset_counter(ic2);
        xt_perf_reset_counter(dc1);
        xt_perf_reset_counter(dc2);
        xt_perf_reset_counter(dc3);
        xt_perf_reset_counter(dc4);
#endif
        effp_process(inp, outp);
#ifdef TEST_PEFMON
        iv1 = xt_perf_counter32(ic1);
        iv2 = xt_perf_counter32(ic2);
        dv1 = xt_perf_counter32(dc1);
        dv2 = xt_perf_counter32(dc2);
        dv3 = xt_perf_counter32(dc3);
        dv4 = xt_perf_counter32(dc4);
        PRINTF_E("[XtPerf] ICACHE hits counts %d, miss counts %d\n", iv1, iv2);
        PRINTF_E("[XtPerf] DCACHE(LOAD+STORE) hits counts %d, miss counts %d\n", dv1+dv3, dv2+dv4);
#endif
        mcps_calc_after_proc(&ffp_mcps);
    }
#ifdef TEST_PEFMON
    xt_perf_disable();
#endif

    if (ffp_mcps.count == 0) {
        PRINTF_E("Zero count.\n");
        return 0;
    }
    PRINTF_E("count:%llu, cc:%llu, latest:%d, avg:%d, max:%d, min:%d\n",
             ffp_mcps.count,
             ffp_mcps.total_cc,
             ffp_mcps.stop-ffp_mcps.start,
             (uint32_t)(ffp_mcps.total_cc / ffp_mcps.count),
             ffp_mcps.max,
             ffp_mcps.min);

    effp_uninit();
    return 0;
}

static const CLI_Command_Definition_t cli_cmd_effp_test_cmd =
{
    "effp",
    "\r\neffp: run effp mcps test r\n",
    cli_cmd_effp_test,
    -1
};

static void cli_effp_test_register(void)
{
    FreeRTOS_CLIRegisterCommand(&cli_cmd_effp_test_cmd);
}

void effp_test_init(void)
{
    cli_effp_test_register();
}
