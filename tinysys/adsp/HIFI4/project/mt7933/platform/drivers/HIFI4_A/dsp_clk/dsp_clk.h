#ifndef _DSP_CLK_H_
#define _DSP_CLK_H_

#include "types.h"

#define REG_TOP_CLK_BASE    (0x30020000)
#define REG_TOP_PLL_BASE    (0x300C0000)

#define REG_PLL_CLK_CG      (REG_TOP_CLK_BASE + 0x200)
#define REG_DSP_CLK_CTL    (REG_TOP_CLK_BASE + 0x230)
#define REG_DSPPLL_CON0           (REG_TOP_PLL_BASE + 0x30)
#define REG_DSPPLL_CON1           (REG_TOP_PLL_BASE + 0x34)
#define REG_DSPPLL_CON2           (REG_TOP_PLL_BASE + 0x38)
#define REG_DSPPLL_CON3           (REG_TOP_PLL_BASE + 0x3C)
#define REG_DSPPLL_FLOWCTRL    (REG_TOP_PLL_BASE + 0xC4)

#define DSPPLL_CG   BIT(1)
#define DSPPLL_EN   BIT(5)
#define DSPPLL_RDV   BIT(1)

#define MHZ(_x_) (_x_##000000)

#define PLL_CR1_CTL_BITMASK (0x73000000)

/* DSP Clock Mux value */
#define CLK_DSP_SEL_26M               0
#define CLK_DSP_SEL_13M               1
#define CLK_DSP_SEL_32K               2
#define CLK_DSP_SEL_DSPPLL            3
#define CLK_DSP_SEL_DSPPLL_D2            4
#define CLK_DSP_SEL_DSPPLL_D4            5
#define CLK_DSP_SEL_DSPPLL_D8            6

#define DSP_CLKSEL_FIELD(_rate, _src_rate, _src_type, _sel) \
    (((_rate) & 0xFFF) << 20 | ((_src_rate) & 0xFFF) << 8 | ((_src_type) & 0xF) << 4 | ((_sel) & 0xF))
#define DSP_CLKSEL_FIELD_RATE(_fld) (((_fld) >> 20) & 0xFFF)
#define DSP_CLKSEL_FIELD_SRC_RATE(_fld) (((_fld) >> 8) & 0xFFF)
#define DSP_CLKSEL_FIELD_SRC_TYPE(_fld) (((_fld) >> 4) & 0xF)
#define DSP_CLKSEL_FIELD_SEL(_fld) ((_fld) & 0xF)

typedef enum {
    DSP_CLK_TYPE_XTAL = 0,
    DSP_CLK_TYPE_DSPPLL,
} dsp_clk_src_t;

/* If ADD NEW ITEM, MAKE SURE FROM LITTLE TO LARGE */
typedef enum {
    DSP_CLK_13M                 = DSP_CLKSEL_FIELD(13, 26, DSP_CLK_TYPE_XTAL, CLK_DSP_SEL_13M),
    DSP_CLK_26M                 = DSP_CLKSEL_FIELD(26, 26, DSP_CLK_TYPE_XTAL, CLK_DSP_SEL_26M),
    DSP_CLK_300M               = DSP_CLKSEL_FIELD(300, 300, DSP_CLK_TYPE_DSPPLL, CLK_DSP_SEL_DSPPLL),
    /* DSP_CLK_300M_D          = DSP_CLKSEL_FIELD(300, 600, DSP_CLK_TYPE_DSPPLL, CLK_DSP_SEL_DSPPLL_D2),*/
    DSP_CLK_400M               = DSP_CLKSEL_FIELD(400, 400, DSP_CLK_TYPE_DSPPLL, CLK_DSP_SEL_DSPPLL),
    DSP_CLK_500M               = DSP_CLKSEL_FIELD(500, 500, DSP_CLK_TYPE_DSPPLL, CLK_DSP_SEL_DSPPLL),
    DSP_CLK_600M               = DSP_CLKSEL_FIELD(600, 600, DSP_CLK_TYPE_DSPPLL, CLK_DSP_SEL_DSPPLL),
} dsp_clk_tbl_t;

enum mux_id_t {
    MUX_CLK_DSP_SEL = 0,
    DSP_MUX_NUM,
};

void dsp_clk_select(unsigned int clksel);
void dsp_clk_init(void);

#endif
