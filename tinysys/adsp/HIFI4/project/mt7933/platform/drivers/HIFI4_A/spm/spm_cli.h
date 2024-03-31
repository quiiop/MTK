#ifndef __SPM_CLI_H__
#define __SPM_CLI_H__

#include "mt_reg_base.h"


#define PCM_REG15_DATA                 (SCPSYS_REG_BASE + 0x13C)
#define SPM_SW_RSV_6                   (SCPSYS_REG_BASE + 0x64C)
#define SPM_SW_RSV_9                   (SCPSYS_REG_BASE + 0x658)
#define SPM_DVFS_EVENT_STA             (SCPSYS_REG_BASE + 0x69C)
#define SPM_DFS_LEVEL                  (SCPSYS_REG_BASE + 0x6B0)
#define SPM_DVS_LEVEL                  (SCPSYS_REG_BASE + 0x6B4)

#define DVFSRC_BASIC_CONTROL       		 (DVFSRC_REG_BASE)
#define DVFSRC_SW_REQ2             		 (DVFSRC_REG_BASE + 0x8)
#define DVFSRC_INT                 		 (DVFSRC_REG_BASE + 0x98)
#define DVFSRC_LEVEL               		 (DVFSRC_REG_BASE + 0xDC)
#define DVFSRC_FORCE               		 (DVFSRC_REG_BASE + 0x300)
#define DVFSRC_SEC_SW_REQ          		 (DVFSRC_REG_BASE + 0x304)


#define DSP2SPM_REQ          		 	0x10018224
#define DSP2SPM_REQ_STA          		0x10018228

/* DVFSRC_SW_REQ2 0x8 */
#define EMI_SW_AP2_SHIFT	0
#define EMI_SW_AP2_MASK		0x3
#define VCORE_SW_AP2_SHIFT	2
#define VCORE_SW_AP2_MASK	0x3

#define CURRENT_LEVEL_SHIFT	16
#define CURRENT_LEVEL_MASK	0xFFFF

#define TARGET_FORCE_SHIFT	0
#define TARGET_FORCE_MASK	0xFFFF
#define CURRENT_FORCE_SHIFT	16
#define CURRENT_FORCE_MASK	0xFFFF

#define PCM_DVFS_INI_CMD		0xABCD0000
#define PCM_SUSPEND_INI_CMD		0x50D10000

#define DVFSRC_TIMEOUT		1000

enum vcore_opp {
	VCORE_0P65 = 0,
	VCORE_0P7,
	VCORE_0P8,
	VCORE_OPP_NUM
};

enum ddr_opp {
	DDR_OPP_0 = 0,
	DDR_OPP_1,
	DDR_OPP_2,
	DDR_OPP_NUM
};

#define DVFSRC_VCORE_REQ 0
#define DVFSRC_DDR_REQ 1

void spm_cli_init() NORMAL_SECTION_FUNC;

#ifdef CFG_CLI_SUPPORT
#endif

#endif /* DVFSRC_H */
