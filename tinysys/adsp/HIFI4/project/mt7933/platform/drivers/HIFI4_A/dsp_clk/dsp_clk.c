#include <driver_api.h>
#include "FreeRTOS.h"
#include "task.h"
#include "dsp_clk.h"
#include "systimer.h"

#define clk_readl(addr)				DRV_Reg32(addr)
#define clk_writel(addr, val)			DRV_WriteReg32(addr, val)
#define clk_setl(addr, val)			clk_writel(addr, clk_readl(addr) | (val))
#define clk_clrl(addr, val)			clk_writel(addr, clk_readl(addr) & ~(val))
#define clk_writel_mask(addr, mask, val)	clk_writel(addr, (clk_readl(addr) & ~(mask)) | (val));

struct pll_pcw_t {
    unsigned int mhz;
    unsigned int sdm_pcw;
    unsigned int pll_cr1;
};

static const struct pll_pcw_t dsp_pll_pcw[] = {
    { 300, 0x2E276276, 0x20000000 },
    { 400, 0x3D89D89D, 0x20000000 },
    { 500, 0x26762762, 0x10000000 },
    { 600, 0x2E276276, 0x10000000 }
};
static unsigned int dsp_mux_val;
static unsigned int dsp_pll_rate = 0;
static unsigned int latest_clksel = 0;

static int dsp_mux_sel_set(enum mux_id_t mux_id, uint32_t value)
{
	switch (mux_id) {
	case MUX_CLK_DSP_SEL:
		clk_writel_mask(REG_DSP_CLK_CTL, (0x7 << 0), (value << 0));
		dsp_mux_val = value;
		PRINTF_D("%s [time:%llu], dspclk_mux = %u, value= %u, REG_DSP_CLK_CTL = 0x%08x\n",
			__func__, read_systimer_stamp_ns(), dsp_mux_val, value, clk_readl(REG_DSP_CLK_CTL));
		break;
	default:
		PRINTF_E("%s [time:%llu], error: unknown mux_id (%d)\n",
			__func__, read_systimer_stamp_ns(), mux_id);
		break;
	}

	return 0;
}

static int dsp_mux_sel_get(enum mux_id_t mux_id)
{
	int value = -1;
	switch (mux_id) {
	case MUX_CLK_DSP_SEL:
		value = clk_readl(REG_DSP_CLK_CTL) & 0x7;
		break;
	default:
		break;
	}

	return value;
}

static unsigned int dsp_mux_sel_is_dsppll(void)
{
    int mux = dsp_mux_sel_get(MUX_CLK_DSP_SEL);
    return (( mux == CLK_DSP_SEL_DSPPLL_D8)
        || (mux == CLK_DSP_SEL_DSPPLL_D4)
        || (mux == CLK_DSP_SEL_DSPPLL_D2)
        || (mux == CLK_DSP_SEL_DSPPLL));
}

static unsigned int dsp_pll_is_enabled(void)
{
    return ((clk_readl(REG_TOP_PLL_BASE) & DSPPLL_EN) != 0);
}

static void dsp_pll_enable(void)
{
    unsigned int is_enabled = dsp_pll_is_enabled();
    unsigned long long time_start, time_end;
    unsigned long long TICK_1MS = TIME_TO_TICK_MS(1) * 1000000;

    if (is_enabled) {
        PRINTF_D("[dsp_pll_enable] PLL already enabled\n");
        return;
    }

    clk_setl(REG_TOP_PLL_BASE, DSPPLL_EN);
    time_start = read_systimer_stamp_ns();
    while (!(clk_readl(REG_DSPPLL_FLOWCTRL) & DSPPLL_RDV)) {
        time_end = read_systimer_stamp_ns();
        if (time_end - time_start > TICK_1MS) {
            PRINTF_E("[dsp_pll_enable] PLL enable timeout!\n");
            break;
        }
    }
    clk_clrl(REG_PLL_CLK_CG, DSPPLL_CG);
    clk_setl(REG_PLL_CLK_CG, DSPPLL_CG);
}

static void dsp_pll_disable(void)
{
    unsigned int is_enabled = dsp_pll_is_enabled();
    if (!is_enabled) {
        PRINTF_D("[dsp_pll_disable] PLL already disabled\n");
        return;
    }

    clk_clrl(REG_PLL_CLK_CG, DSPPLL_CG);
    clk_clrl(REG_TOP_PLL_BASE, DSPPLL_EN);
}

static int dsp_pll_set_freq(unsigned int rate)
{
    const struct pll_pcw_t *pcw = &dsp_pll_pcw[0];
    int i;

    /* rate no change, just return */
    if (dsp_pll_rate == rate)
        return 0;


    for (i = 0; i < sizeof(dsp_pll_pcw)/sizeof(struct pll_pcw_t); i++) {
        if (pcw[i].mhz == rate) {
            break;
        }
    }

    if (i == sizeof(dsp_pll_pcw)/sizeof(struct pll_pcw_t)) {
        PRINTF_E("[dsppll_set_freq] Not a support rate!\n");
        return -1;
    }

    clk_writel(REG_DSPPLL_CON3, pcw[i].sdm_pcw);
    clk_writel_mask(REG_DSPPLL_CON1, PLL_CR1_CTL_BITMASK, pcw[i].pll_cr1);

    dsp_pll_rate = rate;
    return 0;
}

void dsp_clk_select(unsigned int clksel)
{
    unsigned int new_src_rate = DSP_CLKSEL_FIELD_SRC_RATE(clksel);
    unsigned int new_src_type = DSP_CLKSEL_FIELD_SRC_TYPE(clksel);
    unsigned int new_sel = DSP_CLKSEL_FIELD_SEL(clksel);
    unsigned int old_src_rate = DSP_CLKSEL_FIELD_SRC_RATE(latest_clksel);

    if (latest_clksel == clksel) {
        PRINTF_D("[dsp_clk_select] clksel no change!\n");
        return;
    }
    /* DSPPLL -> DSPPLL, 1. switch to xtal, 2. disable dsppll, 3. enable dsppll, 4. switch to dsppll */
    /* Other -> DSPPLL, 1. enable dsppll, 2. switch to dsppll */
    /* DSPPLL -> Other, 1. switch to other 2. disable dsppll */
    /* Other -> Other, 1. switch to other */

    if (new_src_type == DSP_CLK_TYPE_DSPPLL) {
        if (dsp_mux_sel_is_dsppll())
            dsp_mux_sel_set(MUX_CLK_DSP_SEL, CLK_DSP_SEL_26M);
        if (old_src_rate != new_src_rate) {
            dsp_pll_disable();
            dsp_pll_set_freq(new_src_rate);
        }
        dsp_pll_enable();
        dsp_mux_sel_set(MUX_CLK_DSP_SEL, new_sel);
    } else {
        dsp_mux_sel_set(MUX_CLK_DSP_SEL, new_sel);
        dsp_pll_disable();
    }
    PRINTF_D("old clksel is 0x%08x\n", latest_clksel);
    PRINTF_D("new clksel is 0x%08x\n", clksel);
    latest_clksel = clksel;
}

#if defined(CFG_DSP_CLK_CLI_SUPPORT)
#include "cli.h"

static int cli_cmd_dsp_clk_dump_info(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    unsigned int rate = DSP_CLKSEL_FIELD_RATE(latest_clksel);
    unsigned int src_rate = DSP_CLKSEL_FIELD_SRC_RATE(latest_clksel);
    unsigned int sel = DSP_CLKSEL_FIELD_SEL(latest_clksel);
    unsigned int is_enabled = dsp_pll_is_enabled();

    PRINTF_I("====== DSP CLOCK INFO ======\n");
    PRINTF_D("Latest clk select is 0x%08x\n", latest_clksel);
    PRINTF_I("DSP Clock Frequency is %dMhz\n", rate);
    PRINTF_I("DSPPLL is %s\n", is_enabled ? "enabled" : "disabled");
    if (is_enabled)
        PRINTF_I("DSPPLL Frequency is %dMhz\n", src_rate);
    PRINTF_I("DSP Clock Mux is %d\n", sel);
    PRINTF_I("=========================\n");
    return 0;
}

static const CLI_Command_Definition_t cli_cmd_dsp_cli_process_cmd =
{
    "dspclk",
    "\r\ndspclk\r\n",
    cli_cmd_dsp_clk_dump_info,
    -1
};

static void cli_dsp_clk_process_register(void)
{
    FreeRTOS_CLIRegisterCommand(&cli_cmd_dsp_cli_process_cmd);
}
#endif

void dsp_clk_init(void)
{
#if defined(CFG_DSP_CLK_CLI_SUPPORT)
    cli_dsp_clk_process_register();
#endif
}


